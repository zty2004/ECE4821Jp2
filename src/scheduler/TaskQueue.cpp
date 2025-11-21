#include "TaskQueue.h"

#include <atomic>
#include <future>
#include <memory>
#include <mutex>
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
  dst.onCompleted = [this]() {
    running.fetch_sub(1, std::memory_order_relaxed);
    completed.fetch_add(1, std::memory_order_relaxed);
  };
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

  running.fetch_add(1, std::memory_order_relaxed);
  fetchTick.fetch_add(1, std::memory_order_relaxed);
  return true;
}
