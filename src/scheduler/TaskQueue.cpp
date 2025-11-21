#include "TaskQueue.h"

#include <atomic>
#include <cstdint>
#include <future>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <utility>

#include "../query/Query.h"
#include "../query/QueryHelpers.h"
#include "../query/QueryPriority.h"
#include "../query/QueryResult.h"
#include "ScheduledItem.h"

auto TaskQueue::registerTask(std::unique_ptr<Query> query)
    -> std::future<std::unique_ptr<QueryResult>> {
  if (!query) {
    throw std::invalid_argument("null query");
  }
  std::lock_guard<std::mutex> lock(mu);
  if (getId(*query) < 0) {
    throw std::invalid_argument("query id not assigned");
  }

  if (quitFlag) {
    auto tmpPromise = std::promise<std::unique_ptr<QueryResult>>();
    auto fut =
        tmpPromise
            .get_future(); // TODO: input should be ParsedQuery with promise
    tmpPromise.set_value(std::make_unique<ErrorMsgResult>(
        "RegisterTask", "system", "quitFlag active: rejecting new task"));
    return fut;
  }

  ScheduledItem item;
  item.seq = static_cast<std::uint64_t>(getId(*query));
  item.priority = classifyPriority(queryType(*query));
  item.tableId = resolveTableId(*query);
  item.type = queryType(*query);
  item.query = std::move(query);

  auto fut = item.promise.get_future();
  submitted.fetch_add(1, std::memory_order_relaxed);

  if (item.type == QueryType::Quit || item.type == QueryType::List) {
    barriers.emplace_back(std::move(item));
    return fut;
  }

  if (item.type == QueryType::Load) {
    item.depends =
        LoadDeps(0, 0, extractFilePath(*item.query)); // TODO: MakeSchedule
    loadQueue.emplace_back(std::move(item));
    return fut;
  }

  if (item.type == QueryType::Dump) {
    item.depends =
        DumpDeps(0, extractFilePath(*item.query)); // TODO: MakeSchedule
  } else if (item.type == QueryType::Drop) {
    item.depends = DropDeps(); // TODO: MakeSchedule
  } else if (item.type == QueryType::CopyTable) {
    item.depends = CopyTableDeps(); // TODO: MakeSchedule
  }

  auto &tbl = tables[item.tableId];
  tbl.queue.emplace_back(std::move(item));
  return fut;
}
