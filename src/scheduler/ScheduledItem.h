#ifndef SRC_SCHEDULER_SCHEDULEDITEM_H_
#define SRC_SCHEDULER_SCHEDULEDITEM_H_

#include <cstdint>
#include <future>
#include <memory>
#include <string>
#include <variant>
#include <vector>

#include "../query/Query.h"
#include "../query/QueryPriority.h"

class Query;
class QueryResult;

struct LoadDeps {
  std::uint64_t fileDependsOn{0};
  std::uint64_t tableDependsOn{0};
  std::string filePath;
  std::vector<std::string> pendingTable;
};
struct DumpDeps {
  std::uint64_t fileDependsOn{0};
  std::string filePath;
  std::uint64_t tableDependsOn{0};
};
struct DropDeps {
  std::uint64_t tableDependsOn{0};
};
struct CopyTableDeps {
  std::uint64_t srcTableDependsOn{0};
  std::uint64_t dstTableDependsOn{0};
  std::string newTable;
};
using DependencyPayload =
    std::variant<std::monostate, LoadDeps, DumpDeps, DropDeps, CopyTableDeps>;

// One scheduled task (move-only)
struct ScheduledItem {
  std::uint64_t seq = 0;  // global submission sequence //NOLINT
  QueryPriority priority = QueryPriority::NORMAL;  // NOLINT
  std::string tableId;              // table name or "__control__" //NOLINT
  QueryType type = QueryType::Nop;  // type of the query //NOLINT
  DependencyPayload depends;     // default is std::monostate (no deps) //NOLINT
  std::unique_ptr<Query> query;  // NOLINT
  std::promise<std::unique_ptr<QueryResult>>
      promise;               // result promise //NOLINT
  bool droppedFlag = false;  // dropped mark//NOLINT

  ScheduledItem() noexcept = default;
  ~ScheduledItem() = default;
  ScheduledItem(ScheduledItem &&) noexcept = default;
  auto operator=(ScheduledItem &&) noexcept -> ScheduledItem & = default;

  ScheduledItem(const ScheduledItem &) = delete;
  auto operator=(const ScheduledItem &) -> ScheduledItem & = delete;
};

#endif  // SRC_SCHEDULER_SCHEDULEDITEM_H_
