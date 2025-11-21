#include "TaskQueue.h"

#include <atomic>
#include <csignal>
#include <exception>
#include <format>
#include <future>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <utility>

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
    DepManager.markScheduled(item, item.type);
    loadQueue.emplace_back(std::move(item));
    return fut;
  }

  if (item.type == QueryType::Dump || item.type == QueryType::Drop ||
      item.type == QueryType::CopyTable) {
    DepManager.markScheduled(item, item.type);
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
  dst.execOverride = nullptr; // normal path executed by worker
  const ActionList actions = classifyActions(src);
  const std::string capturedTable =
      src.tableId; // src.tableId still valid post-move of query & promise
  const QueryType capturedType = src.type;
  const std::uint64_t capturedSeq = src.seq;
  dst.onCompleted = [this, actions, capturedTable, capturedType,
                     capturedSeq]() {
    {
      std::lock_guard<std::mutex> callbackLock(mu);
      ScheduledItem meta; // placeholder
      meta.tableId = capturedTable;
      meta.type = capturedType;
      meta.seq = capturedSeq;
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

// Refactored single-path fetchNext (reduced branching)
auto TaskQueue::fetchNext(ExecutableTask &out) -> bool {
  std::lock_guard<std::mutex> lock(mu);

  if (quitFlag) {
    return false;
  }

  // Acquire LOAD candidate considering barrier
  ScheduledItem *loadCand = nullptr;
  if (!loadQueue.empty() && !loadBlocked) {
    if (!barriers.empty() && loadQueue.front().seq > barriers.front().seq) {
      loadBlocked = true;
    }
    loadCand = &loadQueue.front();
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
  ScheduledItem *chosen = nullptr;
  TableQueue *chosenTableQ = nullptr;
  if (loadCand != nullptr && tableCand != nullptr) {
    const bool preferLoad = (loadCand->priority < tableCand->priority) ||
                            (loadCand->priority == tableCand->priority &&
                             loadCand->seq < tableCand->seq);
    chosen = preferLoad ? loadCand : tableCand;
    if (!preferLoad) {
      chosenTableQ = tableCandQ;
    }
  } else if (loadCand != nullptr) {
    chosen = loadCand;
  } else {
    chosen = tableCand;
    chosenTableQ = tableCandQ;
  }

  // Materialize and update structures
  if (chosen == loadCand) {
    buildExecutableFromScheduled(*chosen, out);
    loadQueue.pop_front();
  } else if (chosenTableQ != nullptr) {
    buildExecutableFromScheduled(*chosen, out);
    chosenTableQ->queue.pop_front();
    if (!chosenTableQ->queue.empty()) {
      const ScheduledItem &newHead = chosenTableQ->queue.front();
      globalIndex.upsert(chosenTableQ, newHead.priority,
                         fetchTick.load(std::memory_order_relaxed),
                         newHead.seq);
    }
  }

  running.fetch_add(1, std::memory_order_relaxed);
  fetchTick.fetch_add(1, std::memory_order_relaxed);
  return true;
}
