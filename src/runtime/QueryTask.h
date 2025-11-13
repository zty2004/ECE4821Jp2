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

enum class OpKind { Read, Write };

struct QueryTask {
  TableId table;
  OpKind kind = OpKind::Read;
  Query::Ptr query;
  std::uint32_t attempts = 0;
  std::size_t orderIndex = 0;

  QueryTask() = default;

  QueryTask(TableId t, OpKind k, Query::Ptr q, std::size_t idx)
      : table(std::move(t)), kind(k), query(std::move(q)), orderIndex(idx) {}

  QueryTask(QueryTask &&) = default;
  QueryTask &operator=(QueryTask &&) = default;

  QueryTask(const QueryTask &) = delete;
  QueryTask &operator=(const QueryTask &) = delete;
};

#endif  // SRC_RUNTIME_QUERYTASK_H_
