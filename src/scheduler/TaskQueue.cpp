#include "TaskQueue.h"

#include <atomic>
#include <csignal>
#include <format>
#include <future>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <utility>
#include <vector>

#include "../db/Database.h"
#include "../query/Query.h"
#include "../query/QueryResult.h"
#include "DependencyManager.h"
#include "ScheduledItem.h"

auto TaskQueue::registerTask(ParsedQuery &&parsedQuery)
    -> std::future<std::unique_ptr<QueryResult>> {
  ParsedQuery prQuery = std::move(parsedQuery);
  std::lock_guard<std::mutex> lock(mu);

  if (quitFlag) {
    auto tmpPromise = std::promise<std::unique_ptr<QueryResult>>();
    auto fut = tmpPromise.get_future();
    tmpPromise.set_value(std::make_unique<ErrorMsgResult>(
        "RegisterTask", "system", "quitFlag active: rejecting new task"));
    return fut;
  }

  ScheduledItem item;
  item.seq = prQuery.seq;
  item.priority = prQuery.priority;
  item.tableId = prQuery.tableName;
  item.type = prQuery.type;
  item.query = std::move(prQuery.query);
  item.promise = std::move(prQuery.promise);

  auto fut = item.promise.get_future();
  submitted.fetch_add(1, std::memory_order_relaxed);

  if (item.type == QueryType::Quit || item.type == QueryType::List) {
    barriers.emplace_back(std::move(item));
    return fut;
  }

  if (item.type == QueryType::Load) {
    depManager.markScheduled(item, item.type);
    loadQueue.emplace_back(std::make_unique<ScheduledItem>(std::move(item)));
    return fut;
  }

  if (item.type == QueryType::Dump || item.type == QueryType::Drop ||
      item.type == QueryType::CopyTable) {
    depManager.markScheduled(item, item.type);
  }

  auto &tbl = tables[item.tableId];
  tbl.queue.emplace_back(std::move(item));
  return fut;
}

void TaskQueue::buildExecutableFromScheduled(ScheduledItem &src,
                                             ExecutableTask &dst) {
  dst.seq = src.seq;
  dst.type = src.type;
  dst.query = std::move(src.query);
  dst.promise = std::move(src.promise);
  dst.execOverride = src.droppedFlag
                         ? []() { return std::make_unique<NullQueryResult>(); }
                         : nullptr;
  const ActionList actions = classifyActions(src);
  const std::string capturedTable =
      src.tableId; // src.tableId still valid post-move of query & promise
  const QueryType capturedType = src.type;
  const std::uint64_t capturedSeq = src.seq;
  const DependencyPayload capturedDeps = src.depends;
  dst.onCompleted = [this, actions, capturedTable, capturedType, capturedSeq,
                     capturedDeps]() {
    {
      std::lock_guard<std::mutex> callbackLock(mu);
      ScheduledItem meta; // placeholder
      meta.tableId = capturedTable;
      meta.type = capturedType;
      meta.seq = capturedSeq;
      meta.depends = capturedDeps;
      applyActions(actions, meta);
    }
    running.fetch_sub(1, std::memory_order_relaxed);
    completed.fetch_add(1, std::memory_order_relaxed);
  };
}

auto TaskQueue::classifyActions(const ScheduledItem &item) -> ActionList {
  ActionList actions;
  switch (item.type) {
  case QueryType::Load:
  case QueryType::CopyTable: {
    auto tblIt = tables.find(item.tableId);
    bool needRegister = true;
    if (tblIt != tables.end() && tblIt->second.registered) {
      if (tblIt->second.registerSeq > item.seq) {
        throw std::invalid_argument(std::format(
            "Wrongly execute a latter Load {} before an ealier Load {}",
            tblIt->second.registerSeq, item.seq));
      }
      needRegister = false;
    }
    if (needRegister) {
      actions.push_back(CompletionAction::RegisterTable);
    }
    actions.push_back(CompletionAction::UpdateDeps);
    break;
  }
  case QueryType::Dump:
  case QueryType::Drop: {
    actions.push_back(CompletionAction::UpdateDeps);
    break;
  }
  default:
    break; // no action for other types
  }
  return actions;
}

void TaskQueue::applyActions(const ActionList &actions,
                             const ScheduledItem &item) {
  for (auto act : actions) {
    switch (act) {
    case CompletionAction::RegisterTable: {
      auto &tbl = tables[item.tableId];
      if (!tbl.registered && !tbl.queue.empty()) {
        tbl.registered = true;
        tbl.registerSeq = item.seq;
        for (auto &pending : tbl.queue) {
          if (pending.seq < item.seq) {
            pending.droppedFlag =
                true; // mark the queries before registered as dropped
          }
        }
        const ScheduledItem &head = tbl.queue.front();
        globalIndex.upsert(&tbl, head.priority,
                           fetchTick.load(std::memory_order_relaxed), head.seq);
      }
      break;
    }
    case CompletionAction::UpdateDeps: {
      std::vector<std::unique_ptr<ScheduledItem>> readyFileItems;
      std::vector<std::unique_ptr<ScheduledItem>> readyTableItems;
      switch (item.type) {
      case QueryType::Load: {
        const auto &loadDeps = std::get<LoadDeps>(item.depends);
        depManager.notifyCompleted(DependencyManager::DependencyType::File,
                                   loadDeps.filePath, item.seq, readyFileItems);
        if (!loadDeps.pendingTable.empty()) {
          for (const auto &ptableId : loadDeps.pendingTable) {
            depManager.notifyCompleted(DependencyManager::DependencyType::Table,
                                       ptableId, item.seq, readyTableItems);
          }
        } else {
          depManager.notifyCompleted(DependencyManager::DependencyType::Table,
                                     item.tableId, item.seq, readyTableItems);
        }
        break;
      }
      case QueryType::Dump: {
        const auto &dumpDeps = std::get<DumpDeps>(item.depends);
        depManager.notifyCompleted(DependencyManager::DependencyType::File,
                                   dumpDeps.filePath, item.seq, readyFileItems);
        break;
      }
      case QueryType::Drop: {
        depManager.notifyCompleted(DependencyManager::DependencyType::Table,
                                   item.tableId, item.seq, readyTableItems);
        break;
      }
      case QueryType::CopyTable: {
        const auto &copyDeps = std::get<CopyTableDeps>(item.depends);
        depManager.notifyCompleted(DependencyManager::DependencyType::Table,
                                   item.tableId, item.seq, readyTableItems);
        depManager.notifyCompleted(DependencyManager::DependencyType::Table,
                                   copyDeps.newTable, item.seq,
                                   readyTableItems);
        break;
      }
      default:
        break;
      }
      for (auto &&readyItem : readyFileItems) {
        if (readyItem->type == QueryType::Load) {
          const auto &rlDeps = std::get<LoadDeps>(readyItem->depends);
          if (!rlDeps.pendingTable.empty()) {
            auto &database = Database::getInstance();
            readyItem->tableId = database.getFileTableName(rlDeps.filePath);
          } else if (rlDeps.tableDependsOn >
                     depManager.lastCompletedFor(
                         DependencyManager::DependencyType::Table,
                         readyItem->tableId)) {
            const auto &readyTableId = readyItem->tableId;
            depManager.addWait(DependencyManager::DependencyType::Table,
                               readyTableId, std::move(readyItem));
            continue;
          }
          loadQueue.push_front(std::move(readyItem));
        } else {
          auto &pendingTable = tables[readyItem->tableId];
          pendingTable.queue.push_front(std::move(*readyItem));
          readyItem.reset();
          const auto &head = pendingTable.queue.front();
          globalIndex.upsert(&pendingTable, head.priority,
                             fetchTick.load(std::memory_order_relaxed),
                             head.seq);
        }
      }
      for (auto &&readyItem : readyTableItems) {
        if (readyItem->type == QueryType::Load) {
          loadQueue.push_front(std::move(readyItem));
        } else {
          bool flag = true;
          if (readyItem->type == QueryType::CopyTable) {
            const auto &rcDeps = std::get<CopyTableDeps>(readyItem->depends);
            const auto &srcTableId = readyItem->tableId;
            if (rcDeps.srcTableDependsOn >
                depManager.lastCompletedFor(
                    DependencyManager::DependencyType::Table, srcTableId)) {
              depManager.addWait(DependencyManager::DependencyType::File,
                                 srcTableId, std::move(readyItem));
              flag = false;
            }
            if (rcDeps.dstTableDependsOn >
                depManager.lastCompletedFor(
                    DependencyManager::DependencyType::Table,
                    rcDeps.newTable)) {
              depManager.addWait(DependencyManager::DependencyType::File,
                                 rcDeps.newTable, std::move(readyItem));
              flag = false;
            }
          }
          if (flag) {
            auto &pendingTable = tables[readyItem->tableId];
            pendingTable.queue.push_front(std::move(*readyItem));
            readyItem.reset();
            const auto &head = pendingTable.queue.front();
            globalIndex.upsert(&pendingTable, head.priority,
                               fetchTick.load(std::memory_order_relaxed),
                               head.seq);
          }
        }
      } // TODO: redundant function, can be optimized
      break;
    }
    }
  }
}

// Refactored single-path fetchNext (reduced branching)
auto TaskQueue::fetchNext(ExecutableTask &out) -> bool {
  std::lock_guard<std::mutex> lock(mu);

  while (!quitFlag) {
    // Acquire LOAD candidate considering barrier
    std::unique_ptr<ScheduledItem> loadCand = nullptr;
    if (!loadQueue.empty() && !loadBlocked) {
      if (!barriers.empty() && loadQueue.front()->seq > barriers.front().seq) {
        loadBlocked = true;
      }
      loadCand = std::move(loadQueue.front());
    }

    // Acquire table candidate via GlobalIndex
    ScheduledItem *tableCand = nullptr;
    TableQueue *tableCandQ = nullptr;
    TableQueue *picked = nullptr;
    while (globalIndex.pickBest(picked)) {
      if (picked == nullptr || picked->queue.empty()) {
        continue;
      }
      ScheduledItem &head = picked->queue.front();
      if (!barriers.empty() && head.seq > barriers.front().seq) {
        waitingTables.push_back(picked);
        continue;
      }
      tableCand = &head;
      tableCandQ = picked;
      break;
    }

    // No candidate case
    if (tableCand == nullptr && loadCand == nullptr) {
      if (!barriers.empty()) {
        ScheduledItem barrierItem = std::move(barriers.front());
        barriers.pop_front();
        if (barrierItem.type == QueryType::Quit) {
          quitFlag = true;
        }
        buildExecutableFromScheduled(barrierItem, out);
        running.fetch_add(1, std::memory_order_relaxed);
        fetchTick.fetch_add(1, std::memory_order_relaxed);
        // Reinsert stashed tables
        for (auto *blocked : waitingTables) {
          if (blocked != nullptr && !blocked->queue.empty()) {
            const ScheduledItem &blockedHead = blocked->queue.front();
            globalIndex.upsert(blocked, blockedHead.priority,
                               fetchTick.load(std::memory_order_relaxed),
                               blockedHead.seq);
          }
        }
        waitingTables.clear();
        loadBlocked = false;
        return true;
      }
      return false;
    }

    // choose higher priority, then smaller seq
    bool preferLoad = true;
    if (loadCand != nullptr && tableCand != nullptr) {
      preferLoad = (loadCand->priority < tableCand->priority) ||
                   (loadCand->priority == tableCand->priority &&
                    loadCand->seq < tableCand->seq);
    } else {
      preferLoad = tableCand == nullptr;
    }

    // Materialize and update structures
    if (preferLoad) {
      const auto &lDeps = std::get<LoadDeps>(loadCand->depends);
      if (lDeps.fileDependsOn >
          depManager.lastCompletedFor(DependencyManager::DependencyType::File,
                                      lDeps.filePath)) {
        depManager.addWait(DependencyManager::DependencyType::File,
                           lDeps.filePath, std::move(loadCand));
        continue;
      }
      if (lDeps.fileDependsOn >
          depManager.lastCompletedFor(DependencyManager::DependencyType::Table,
                                      loadCand->tableId)) {
        const auto &loadTableId = loadCand->tableId;
        depManager.addWait(DependencyManager::DependencyType::File, loadTableId,
                           std::move(loadCand));
        continue;
      }
      buildExecutableFromScheduled(*loadCand, out);
      loadQueue.pop_front();
    } else if (tableCandQ != nullptr) {
      if (tableCand->type == QueryType::Dump) {
        const auto &dumpDeps = std::get<DumpDeps>(tableCand->depends);
        if (dumpDeps.fileDependsOn >
            depManager.lastCompletedFor(DependencyManager::DependencyType::File,
                                        dumpDeps.filePath)) {
          auto waitingP = std::make_unique<ScheduledItem>(
              std::move(tableCandQ->queue.front()));
          tableCandQ->queue.pop_front();
          depManager.addWait(DependencyManager::DependencyType::File,
                             dumpDeps.filePath, std::move(waitingP));
          continue;
        }
      }
      if (tableCand->type == QueryType::Drop) {
        const auto &dropDeps = std::get<DropDeps>(tableCand->depends);
        const auto &tableId = tableCand->tableId;
        if (dropDeps.tableDependsOn >
            depManager.lastCompletedFor(
                DependencyManager::DependencyType::Table, tableId)) {
          auto waitingP = std::make_unique<ScheduledItem>(
              std::move(tableCandQ->queue.front()));
          tableCandQ->queue.pop_front();
          depManager.addWait(DependencyManager::DependencyType::File, tableId,
                             std::move(waitingP));
          continue;
        }
      }
      if (tableCand->type == QueryType::CopyTable) {
        const auto &copyDeps = std::get<CopyTableDeps>(tableCand->depends);
        const auto &srcTableId = tableCand->tableId;
        if (copyDeps.srcTableDependsOn >
            depManager.lastCompletedFor(
                DependencyManager::DependencyType::Table, srcTableId)) {
          auto waitingP = std::make_unique<ScheduledItem>(
              std::move(tableCandQ->queue.front()));
          tableCandQ->queue.pop_front();
          depManager.addWait(DependencyManager::DependencyType::File,
                             srcTableId, std::move(waitingP));
          continue;
        }
        if (copyDeps.dstTableDependsOn >
            depManager.lastCompletedFor(
                DependencyManager::DependencyType::Table, copyDeps.newTable)) {
          auto waitingP = std::make_unique<ScheduledItem>(
              std::move(tableCandQ->queue.front()));
          tableCandQ->queue.pop_front();
          depManager.addWait(DependencyManager::DependencyType::File,
                             copyDeps.newTable, std::move(waitingP));
          continue;
        }
      }
      buildExecutableFromScheduled(*tableCand, out);
      tableCandQ->queue.pop_front();
      if (!tableCandQ->queue.empty()) {
        const ScheduledItem &newHead = tableCandQ->queue.front();
        globalIndex.upsert(tableCandQ, newHead.priority,
                           fetchTick.load(std::memory_order_relaxed),
                           newHead.seq);
      }
    }

    running.fetch_add(1, std::memory_order_relaxed);
    fetchTick.fetch_add(1, std::memory_order_relaxed);
    return true;
  }
  return false;
}
