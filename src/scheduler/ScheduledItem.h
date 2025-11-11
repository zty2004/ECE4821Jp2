#ifndef SRC_SCHEDULER_SCHEDULEDITEM_H_
#define SRC_SCHEDULER_SCHEDULEDITEM_H_

#include <cstdint>
#include <future>
#include <memory>
#include <string>

#include "../query/QueryPriority.h"

class Query;
class QueryResult;

// One scheduled task (move-only)
struct ScheduledItem {
  std::uint64_t seq = 0; // global submission sequence
  QueryPriority priority = QueryPriority::NORMAL;
  std::string tableId;                // table name or "__control__"
  std::string filePath;               // optional for LOAD/DUMP
  std::uint64_t dependsOnFileSeq = 0; // 0 means no dependency
  std::unique_ptr<Query> query;
  std::promise<std::unique_ptr<QueryResult>> promise; // result promise
  bool droppedFlag = false;                           // dropped mark

  ScheduledItem() noexcept = default; // NOLINT
  ~ScheduledItem() = default;
  ScheduledItem(ScheduledItem &&) noexcept = default;
  ScheduledItem &operator=(ScheduledItem &&) noexcept = default; // NOLINT

  ScheduledItem(const ScheduledItem &) = delete;
  ScheduledItem &operator=(const ScheduledItem &) = delete; // NOLINT
};

#endif // SRC_SCHEDULER_SCHEDULEDITEM_H_
