#ifndef SRC_QUERY_QUERYHELPERS_H_
#define SRC_QUERY_QUERYHELPERS_H_

#include <string>

#include "./Query.h"
#include "./QueryPriority.h"
#include "management/DumpTableQuery.h"
#include "management/LoadTableQuery.h"

// Resolve a queue routing id for a query
// - If query has a concrete table name then return it
// - Otherwise route to control pseudo table "__control__"
inline auto resolveTableId(const Query &query) -> std::string {
  const auto &table = query.table();
  return table.empty() ? controlTableId() : table;
}

// Extract file path for file-related queries (LOAD/DUMP); return empty if none.
inline auto extractFilePath(const Query &query) -> std::string {
  if (const auto *pLoad = dynamic_cast<const LoadTableQuery *>(&query)) {
    return pLoad->filename();
  }
  if (const auto *pDump = dynamic_cast<const DumpTableQuery *>(&query)) {
    return pDump->filename();
  }
  return {};
}

#endif // SRC_QUERY_QUERYHELPERS_H_
