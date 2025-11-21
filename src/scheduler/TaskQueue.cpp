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
