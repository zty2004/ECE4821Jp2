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
#include "FileDependencyManager.h"
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

  ScheduledItem item;
  item.seq = static_cast<std::uint64_t>(getId(*query));
  item.priority = classifyPriority(queryType(*query));
  item.tableId = resolveTableId(*query);
  item.filePath = extractFilePath(*query);
  item.query = std::move(query);

  auto fut = item.promise.get_future();
  submitted.fetch_add(1, std::memory_order_relaxed);

  QueryType const qtype = queryType(*item.query);
  if (qtype == QueryType::Quit || qtype == QueryType::List) {
    barriers.emplace_back(std::move(item));
    return fut;
  }

  if (qtype == QueryType::Load) {
    FileDependencyManager::LoadNode loadNode; // manual populate
    loadNode.filePath = item.filePath;
    loadNode.dependsOn = 0; // TODO: markScheduled
    loadNode.item = std::move(item);
    loadQueue.emplace_back(std::move(loadNode));
    return fut;
  }

  auto &tbl = tables[item.tableId];
  tbl.queue.emplace_back(std::move(item));
  return fut;
}
