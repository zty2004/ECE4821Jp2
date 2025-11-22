#include "TaskQueue.h"

#include <atomic>
#include <cstdint>
#include <future>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "../db/Database.h"
#include "../query/Query.h"
#include "../query/QueryResult.h"
#include "DependencyManager.h"
#include "ScheduledItem.h"
#include "TableQueue.h"

void TaskQueue::setReady() {
  readyToFetch_.store(true, std::memory_order_release);
}

auto TaskQueue::registerTask(ParsedQuery &&parsedQuery)
    -> std::future<std::unique_ptr<QueryResult>> {
  ParsedQuery prQuery = std::move(parsedQuery);

  // Get future from the promise that Runtime created
  auto fut = prQuery.promise.get_future();

  std::scoped_lock const lock(mu);

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
                     capturedDeps, capturedTableQ]() noexcept -> void {
    {
      std::scoped_lock const callbackLock(mu);
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
    bool needRegister = true;
    // For COPYTABLE, always need RegisterTable to register the new table
    if (item.type == QueryType::Load) {
      // For LOAD, check if the table is already registered
      auto tblIt = tables.find(item.tableId);
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

auto TaskQueue::fetchNext(ExecutableTask &out) -> bool {
  // Don't fetch until all tasks are registered
  if (!readyToFetch_.load(std::memory_order_acquire)) {
    return false;
  }

  std::scoped_lock const lock(mu);

  while (true) {  // Changed from while (!quitFlag) to process all tasks
    std::unique_ptr<ScheduledItem> loadCand = nullptr;
    ScheduledItem *tableCand = nullptr;
    TableQueue *tableCandQ = nullptr;

    getFetched(loadCand, tableCand, tableCandQ);

    // No candidate case
    if (tableCand == nullptr && loadCand == nullptr) {
      return fetchBarrier(out);
    }

    // choose higher priority, then smaller seq
    bool preferLoad = tableCand == nullptr;
    if (loadCand != nullptr && tableCand != nullptr) {
      preferLoad = (loadCand->priority < tableCand->priority) ||
                   (loadCand->priority == tableCand->priority &&
                    loadCand->seq < tableCand->seq);
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
