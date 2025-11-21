#ifndef SRC_SCHEDULER_SCHEDULEDITEM_H_
#define SRC_SCHEDULER_SCHEDULEDITEM_H_

#include <cstdint>
#include <future>
#include <memory>
#include <string>
#include <variant>
#include <vector>

#include "../query/QueryPriority.h"

class Query;
class QueryResult;

struct LoadDeps {
  std::uint64_t fileDependsOn = 0;
  std::uint64_t tableDependsOn = 0;
  std::string filePath;
  std::vector<std::string> pendingTable;
};
struct DumpDeps {
  std::uint64_t fileDependsOn = 0;
  std::string filePath;
};
struct DropDeps {
  std::uint64_t tableDependsOn = 0;
};
struct CopyTableDeps {
  std::uint64_t srcTableDependsOn = 0;
  std::uint64_t dstTableDependsOn = 0;
};
using DependencyPayload =
    std::variant<std::monostate, LoadDeps, DumpDeps, DropDeps, CopyTableDeps>;

// One scheduled task (move-only)
struct ScheduledItem {
  std::uint64_t seq = 0; // global submission sequence
  QueryPriority priority = QueryPriority::NORMAL;
  std::string tableId;         // table name or "__control__"
  QueryType type;              // type of the query
  DependencyPayload depends{}; // default is std::monostate (no deps)
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
