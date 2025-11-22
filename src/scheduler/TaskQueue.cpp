#include "TaskQueue.h"

#include <atomic>
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

void TaskQueue::setReady() {
  readyToFetch_.store(true, std::memory_order_release);
}

auto TaskQueue::registerTask(ParsedQuery &&parsedQuery)
    -> std::future<std::unique_ptr<QueryResult>> {
  ParsedQuery prQuery = std::move(parsedQuery);

  // Get future from the promise that Runtime created
  auto fut = prQuery.promise.get_future();

  std::lock_guard<std::mutex> lock(mu);

  if (quitFlag) {
    auto tmpPromise = std::promise<std::unique_ptr<QueryResult>>();
    auto tmpFut = tmpPromise.get_future();
    tmpPromise.set_value(std::make_unique<ErrorMsgResult>(
        "RegisterTask", "system", "quitFlag active: rejecting new task"));
    return tmpFut;
  }

  ScheduledItem item;
  item.seq = prQuery.seq;
  item.priority = prQuery.priority;
  item.tableId = prQuery.tableName;
  item.type = prQuery.type;
  item.query = std::move(prQuery.query);
  item.promise = std::move(prQuery.promise);

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

  auto &tblPtr = tables[item.tableId];
  if (!tblPtr) {
    tblPtr = std::make_unique<TableQueue>();
  }
  tblPtr->queue.emplace_back(std::move(item));
  return fut;
}

void TaskQueue::buildExecutableFromScheduled(ScheduledItem &src,
                                             ExecutableTask &dst) {
  dst.seq = src.seq;
  dst.type = src.type;
  dst.query = std::move(src.query);
  dst.promise = std::move(src.promise);
  dst.execOverride = src.droppedFlag ? []() -> std::unique_ptr<QueryResult> {
    return std::make_unique<NullQueryResult>();
  }
  : nullptr;
  const ActionList actions = classifyActions(src);
  const std::string capturedTable =
      src.tableId;  // src.tableId still valid post-move of query & promise
  const QueryType capturedType = src.type;
  const std::uint64_t capturedSeq = src.seq;
  const DependencyPayload capturedDeps = src.depends;
  // Find the TableQueue for this task (if it's a table-based query)
  TableQueue *capturedTableQ = nullptr;
  if (!capturedTable.empty() && capturedTable != "__control__") {
    auto tblIt = tables.find(capturedTable);
    if (tblIt != tables.end()) {
      capturedTableQ = tblIt->second.get();
    }
  }
  dst.onCompleted = [this, actions, capturedTable, capturedType, capturedSeq,
                     capturedDeps, capturedTableQ]() -> void {
    {
      std::lock_guard<std::mutex> callbackLock(mu);
      ScheduledItem meta;  // placeholder
      meta.tableId = capturedTable;
      meta.type = capturedType;
      meta.seq = capturedSeq;
      meta.depends = capturedDeps;
      applyActions(actions, meta);
      // Upsert next task from the same table queue (if any)
      if (capturedTableQ != nullptr && !capturedTableQ->queue.empty()) {
        const ScheduledItem &newHead = capturedTableQ->queue.front();
        globalIndex.upsert(capturedTableQ, newHead.priority,
                           fetchTick.load(std::memory_order_relaxed),
                           newHead.seq);
      }
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
    bool needRegister = false;

    if (item.type == QueryType::CopyTable) {
      // For COPYTABLE, always need RegisterTable to register the new table
      needRegister = true;
    } else {
      // For LOAD, check if the table is already registered
      auto tblIt = tables.find(item.tableId);
      needRegister = true;
      if (tblIt != tables.end() && tblIt->second && tblIt->second->registered) {
        if (tblIt->second->registerSeq > item.seq) {
          throw std::invalid_argument(
              "Wrongly execute a later Load " +
              std::to_string(tblIt->second->registerSeq) +
              " before an earlier Load " + std::to_string(item.seq));
        }
        needRegister = false;
      }
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
    // All other table queries should also update dependencies
    if (!item.tableId.empty()) {
      actions.push_back(CompletionAction::UpdateDeps);
    }
    break;
  }
  return actions;
}

void TaskQueue::applyActions(const ActionList &actions,
                             const ScheduledItem &item) {
  for (auto act : actions) {
    switch (act) {
    case CompletionAction::RegisterTable: {
      applyRegisterTable(item);
      break;
    }
    case CompletionAction::UpdateDeps: {
      applyUpdateDeps(item);
      break;
    }
    }
  }
}

void TaskQueue::applyRegisterTable(const ScheduledItem &item) {
  // Register the main table (item.tableId)
  auto &tblPtr = tables[item.tableId];
  if (!tblPtr) {
    tblPtr = std::make_unique<TableQueue>();
  }

  registerTableQueue(tblPtr, item);

  // For COPYTABLE, also register the new table
  if (item.type == QueryType::CopyTable) {
    const auto &copyDeps = std::get<CopyTableDeps>(item.depends);
    const std::string &newTable = copyDeps.newTable;
    auto &newTblPtr = tables[newTable];
    if (!newTblPtr) {
      newTblPtr = std::make_unique<TableQueue>();
    }
    registerTableQueue(newTblPtr, item);
  }
}

void TaskQueue::registerTableQueue(std::unique_ptr<TableQueue> &tblPtr,
                                   const ScheduledItem &item) {
  TableQueue &tbl = *tblPtr;
  if (!tbl.registered) {
    tbl.registered = true;
    tbl.registerSeq = item.seq;
    if (!tbl.queue.empty()) {
      for (auto &pending : tbl.queue) {
        if (pending.seq < item.seq) {
          pending.droppedFlag =
              true;  // mark the queries before registered as dropped
        }
      }
      const ScheduledItem &head = tbl.queue.front();
      globalIndex.upsert(tblPtr.get(), head.priority,
                         fetchTick.load(std::memory_order_relaxed), head.seq);
    }
  }
}

void TaskQueue::applyUpdateDeps(const ScheduledItem &item) {
  std::vector<std::unique_ptr<ScheduledItem>> readyFileItems;
  std::vector<std::unique_ptr<ScheduledItem>> readyTableItems;
  updateDependencyRecords(item, readyFileItems, readyTableItems);
  for (auto &&readyItem : readyFileItems) {
    updateReadyFiles(readyItem);
  }
  for (auto &&readyItem : readyTableItems) {
    updateReadyTables(readyItem);
  }
}

void TaskQueue::updateDependencyRecords(
    const ScheduledItem &item,
    std::vector<std::unique_ptr<ScheduledItem>> &readyFileItems,
    std::vector<std::unique_ptr<ScheduledItem>> &readyTableItems) {
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
    // Also notify table dependency completion
    depManager.notifyCompleted(DependencyManager::DependencyType::Table,
                               item.tableId, item.seq, readyTableItems);
    break;
  }
  case QueryType::Drop: {
    depManager.notifyCompleted(DependencyManager::DependencyType::Table,
                               item.tableId, item.seq, readyTableItems);
    // Clear the registered flag so that future LOADs can re-register the
    // table
    auto tblIt = tables.find(item.tableId);
    if (tblIt != tables.end() && tblIt->second) {
      tblIt->second->registered = false;
    }
    break;
  }
  case QueryType::CopyTable: {
    const auto &copyDeps = std::get<CopyTableDeps>(item.depends);
    depManager.notifyCompleted(DependencyManager::DependencyType::Table,
                               item.tableId, item.seq, readyTableItems);
    depManager.notifyCompleted(DependencyManager::DependencyType::Table,
                               copyDeps.newTable, item.seq, readyTableItems);
    break;
  }
  default:
    // For other query types (INSERT, SELECT, DELETE, UPDATE, etc.),
    // update table dependency
    if (!item.tableId.empty()) {
      depManager.notifyCompleted(DependencyManager::DependencyType::Table,
                                 item.tableId, item.seq, readyTableItems);
    }
    break;
  }
}

void TaskQueue::updateReadyFiles(std::unique_ptr<ScheduledItem> &readyItem) {
  if (readyItem->type == QueryType::Load) {
    const auto &rlDeps = std::get<LoadDeps>(readyItem->depends);
    if (!rlDeps.pendingTable.empty()) {
      auto &database = Database::getInstance();
      readyItem->tableId = database.getFileTableName(rlDeps.filePath);
      // No need to update `tableDependsOn`, since cannot get correct
      // dependency. Ignoring Table dependency might cause issues, but the
      // operations to table also protected by outside mutex, acceptable
    } else if (rlDeps.tableDependsOn >
               depManager.lastCompletedFor(
                   DependencyManager::DependencyType::Table,
                   readyItem->tableId)) {
      // judge if table dependency satisfied for decided LOADs
      const auto &readyTableId = readyItem->tableId;
      depManager.addWait(DependencyManager::DependencyType::Table, readyTableId,
                         std::move(readyItem));
      return;
    }
    loadQueue.emplace_front(std::move(readyItem));
    return;
  }
  auto &pendingTablePtr = tables[readyItem->tableId];
  if (!pendingTablePtr) {
    pendingTablePtr = std::make_unique<TableQueue>();
  }
  auto &pendingTable = *pendingTablePtr;
  pendingTable.queue.push_front(std::move(*readyItem));
  readyItem.reset();
  const auto &head = pendingTable.queue.front();
  globalIndex.upsert(pendingTablePtr.get(), head.priority,
                     fetchTick.load(std::memory_order_relaxed), head.seq);
}

void TaskQueue::updateReadyTables(std::unique_ptr<ScheduledItem> &readyItem) {
  if (readyItem->type == QueryType::Load) {
    loadQueue.emplace_front(std::move(readyItem));
    return;
  }

  if (readyItem->type == QueryType::CopyTable) {
    const auto &rcDeps = std::get<CopyTableDeps>(readyItem->depends);
    const auto &srcTableId = readyItem->tableId;
    if (rcDeps.srcTableDependsOn >
        depManager.lastCompletedFor(DependencyManager::DependencyType::Table,
                                    srcTableId)) {
      depManager.addWait(DependencyManager::DependencyType::Table, srcTableId,
                         std::move(readyItem));
      return;
    }
    if (rcDeps.dstTableDependsOn >
        depManager.lastCompletedFor(DependencyManager::DependencyType::Table,
                                    rcDeps.newTable)) {
      depManager.addWait(DependencyManager::DependencyType::Table,
                         rcDeps.newTable, std::move(readyItem));
      return;
    }
  }
  // DROP directly goes here
  auto &pendingTablePtr = tables[readyItem->tableId];
  if (!pendingTablePtr) {
    pendingTablePtr = std::make_unique<TableQueue>();
  }
  auto &pendingTable = *pendingTablePtr;
  pendingTable.queue.push_front(std::move(*readyItem));
  readyItem.reset();
  const auto &head = pendingTable.queue.front();
  globalIndex.upsert(pendingTablePtr.get(), head.priority,
                     fetchTick.load(std::memory_order_relaxed), head.seq);
}

// Refactored single-path fetchNext (reduced branching)
auto TaskQueue::fetchNext(ExecutableTask &out) -> bool {
  // Don't fetch until all tasks are registered
  if (!readyToFetch_.load(std::memory_order_acquire)) {
    return false;
  }

  std::lock_guard<std::mutex> lock(mu);

  while (true) {  // Changed from while (!quitFlag) to process all tasks
    // Acquire LOAD candidate considering barrier
    std::unique_ptr<ScheduledItem> loadCand = nullptr;
    if (!loadQueue.empty() && !loadBlocked) {
      if (!barriers.empty() && loadQueue.front()->seq > barriers.front().seq) {
        loadBlocked = true;
      } else {
        loadCand = std::move(loadQueue.front());
        loadQueue.pop_front();
      }
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
      return fetchBarrier(out);
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
      if (judgeLoadDeps(loadCand)) {
        continue;
      }
      // loadCand is already moved out from loadQueue at line 291
      buildExecutableFromScheduled(*loadCand, out);
      running.fetch_add(1, std::memory_order_relaxed);
      fetchTick.fetch_add(1, std::memory_order_relaxed);
      return true;
    }
    if (tableCandQ != nullptr) {
      if (judgeNormalDeps(tableCand, tableCandQ)) {
        continue;
      }
      buildExecutableFromScheduled(*tableCand, out);
      tableCandQ->queue.pop_front();
      // Don't upsert next task here - will be done in onCompleted to prevent
      // concurrent execution
    }

    running.fetch_add(1, std::memory_order_relaxed);
    fetchTick.fetch_add(1, std::memory_order_relaxed);
    return true;
  }
  return false;
}

auto TaskQueue::fetchBarrier(ExecutableTask &out) -> bool {
  if (!barriers.empty()) {
    // Barrier must wait for:
    // 1. All running tasks to complete
    // 2. All loads in loadQueue to be processed (or we'd return before
    // this)
    auto runningCount = running.load(std::memory_order_relaxed);
    if (runningCount > 0) {
      return false;  // Cannot process barrier yet, tasks still running
    }

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

auto TaskQueue::judgeLoadDeps(std::unique_ptr<ScheduledItem> &loadCand)
    -> bool {
  const auto &lDeps = std::get<LoadDeps>(loadCand->depends);
  if (lDeps.fileDependsOn >
      depManager.lastCompletedFor(DependencyManager::DependencyType::File,
                                  lDeps.filePath)) {
    depManager.addWait(DependencyManager::DependencyType::File, lDeps.filePath,
                       std::move(loadCand));
    return true;
  }
  if (lDeps.tableDependsOn >
      depManager.lastCompletedFor(DependencyManager::DependencyType::Table,
                                  loadCand->tableId)) {
    const auto &loadTableId = loadCand->tableId;
    depManager.addWait(DependencyManager::DependencyType::Table, loadTableId,
                       std::move(loadCand));
    return true;
  }
  return false;
}

auto TaskQueue::judgeNormalDeps(ScheduledItem *tableCand,
                                TableQueue *tableCandQ) -> bool {
  if (tableCand->type == QueryType::Dump) {
    const auto &dumpDeps = std::get<DumpDeps>(tableCand->depends);
    const std::string filePath = dumpDeps.filePath;  // Copy before move!
    const std::string tableId = tableCand->tableId;  // Copy before move!
    if (dumpDeps.fileDependsOn >
        depManager.lastCompletedFor(DependencyManager::DependencyType::File,
                                    filePath)) {
      auto waitingP =
          std::make_unique<ScheduledItem>(std::move(tableCandQ->queue.front()));
      tableCandQ->queue.pop_front();
      depManager.addWait(DependencyManager::DependencyType::File, filePath,
                         std::move(waitingP));
      return true;
    }
    // Also check table dependency
    if (dumpDeps.tableDependsOn >
        depManager.lastCompletedFor(DependencyManager::DependencyType::Table,
                                    tableId)) {
      auto waitingP =
          std::make_unique<ScheduledItem>(std::move(tableCandQ->queue.front()));
      tableCandQ->queue.pop_front();
      depManager.addWait(DependencyManager::DependencyType::Table, tableId,
                         std::move(waitingP));
      return true;
    }
  }
  if (tableCand->type == QueryType::Drop) {
    const auto &dropDeps = std::get<DropDeps>(tableCand->depends);
    const std::string tableId = tableCand->tableId;  // Copy before move!
    auto lastCompleted = depManager.lastCompletedFor(
        DependencyManager::DependencyType::Table, tableId);
    if (dropDeps.tableDependsOn > lastCompleted) {
      auto waitingP =
          std::make_unique<ScheduledItem>(std::move(tableCandQ->queue.front()));
      tableCandQ->queue.pop_front();
      depManager.addWait(DependencyManager::DependencyType::Table, tableId,
                         std::move(waitingP));
      return true;
    }
  }
  if (tableCand->type == QueryType::CopyTable) {
    const auto &copyDeps = std::get<CopyTableDeps>(tableCand->depends);
    const std::string srcTableId = tableCand->tableId;  // Copy before move!
    const std::string newTable = copyDeps.newTable;     // Copy before move!
    if (copyDeps.srcTableDependsOn >
        depManager.lastCompletedFor(DependencyManager::DependencyType::Table,
                                    srcTableId)) {
      auto waitingP =
          std::make_unique<ScheduledItem>(std::move(tableCandQ->queue.front()));
      tableCandQ->queue.pop_front();
      depManager.addWait(DependencyManager::DependencyType::Table, srcTableId,
                         std::move(waitingP));
      return true;
    }
    if (copyDeps.dstTableDependsOn >
        depManager.lastCompletedFor(DependencyManager::DependencyType::Table,
                                    newTable)) {
      auto waitingP =
          std::make_unique<ScheduledItem>(std::move(tableCandQ->queue.front()));
      tableCandQ->queue.pop_front();
      depManager.addWait(DependencyManager::DependencyType::Table, newTable,
                         std::move(waitingP));
      return true;
    }
  }
  return false;
}
