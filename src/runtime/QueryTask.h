//
// QueryTask - Task representation for query execution
//

#ifndef SRC_RUNTIME_QUERYTASK_H_
#define SRC_RUNTIME_QUERYTASK_H_

#include <cstdint>
#include <memory>
#include <string>
#include <utility>

#include "../query/Query.h"
#include "../query/QueryResult.h"

using TableId = std::string;

enum class OpKind : std::uint8_t { Read, Write };

struct QueryTask {
  // NOLINTBEGIN(misc-non-private-member-variables-in-classes)
  TableId table;
  OpKind kind = OpKind::Read;
  Query::Ptr query;
  std::uint32_t attempts = 0;
  std::size_t orderIndex = 0;
  // NOLINTEND(misc-non-private-member-variables-in-classes)

  QueryTask() = default;
  ~QueryTask() = default;

  QueryTask(TableId tableArg, OpKind kindArg, Query::Ptr queryArg,
            std::size_t idx)
      : table(std::move(tableArg)), kind(kindArg), query(std::move(queryArg)),
        orderIndex(idx) {}

  QueryTask(QueryTask &&) = default;
  auto operator=(QueryTask &&) -> QueryTask & = default;

  QueryTask(const QueryTask &) = delete;
  auto operator=(const QueryTask &) -> QueryTask & = delete;
};

#endif  // SRC_RUNTIME_QUERYTASK_H_
