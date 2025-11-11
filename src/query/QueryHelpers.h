#ifndef SRC_QUERY_QUERYHELPERS_H_
#define SRC_QUERY_QUERYHELPERS_H_

#include <string>

#include "Query.h"
#include "QueryPriority.h"

// Resolve a queue routing id for a query
// - If query has a concrete table name then return it
// - Otherwise route to control pseudo table "__control__"
inline auto resolveTableId(const Query &query) -> std::string {
  const auto &table = query.table();
  return table.empty() ? controlTableId() : table;
}

#endif // SRC_QUERY_QUERYHELPERS_H_
