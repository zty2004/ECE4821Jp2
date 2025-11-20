#ifndef SRC_QUERY_QUERYPRIORITY_H_
#define SRC_QUERY_QUERYPRIORITY_H_

#include "Query.h"
#include <cstdint>
#include <string>

// Scheduling priority for Cross-table usgae
enum class QueryPriority : std::uint8_t {
  SYSTEM_HIGH, // NEVER USED, placeholder
  HIGH,
  NORMAL,
  LOW // reserved
};

// Classify a query type to a priority
inline auto classifyPriority(QueryType qt) -> QueryPriority {
  switch (qt) {
  case QueryType::Quit:
  case QueryType::List:
    return QueryPriority::SYSTEM_HIGH;
  case QueryType::Load:
  case QueryType::Dump:
  case QueryType::Drop:
  case QueryType::Truncate:
  case QueryType::CopyTable:
    return QueryPriority::HIGH;
  default:
    return QueryPriority::NORMAL;
  }
}

// Special control pseudo table for commands without a concrete table name
inline auto controlTableId() -> const std::string & {
  static const std::string pseudoId = "__control__";
  return pseudoId;
}

#endif // SRC_QUERY_QUERYPRIORITY_H_
