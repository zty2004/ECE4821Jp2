#ifndef SRC_QUERY_QUERYPRIORITY_H_
#define SRC_QUERY_QUERYPRIORITY_H_

#include <cstdint>
#include <string>

class Query;

// Scheduling priority for Cross-table usgae
enum class QueryPriority : std::uint8_t {
  SYSTEM_HIGH,
  HIGH,
  NORMAL,
  LOW // reserved
};

// Classify a query to a priority
auto classifyPriority(const Query &query) -> QueryPriority;

// Special control pseudo table for commands without a concrete table name
inline auto controlTableId() -> const std::string & {
  static const std::string pseudoId = "__control__";
  return pseudoId;
}

#endif // SRC_QUERY_QUERYPRIORITY_H_
