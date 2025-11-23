#ifndef SRC_QUERY_QUERYHELPERS_H_
#define SRC_QUERY_QUERYHELPERS_H_

#include <string>

#include "./Query.h"
#include "./QueryPriority.h"

enum class QueryKind : std::uint8_t { Read, Write, Null };

[[nodiscard]] constexpr auto getQueryKind(QueryType type) -> QueryKind {
  switch (type) {
  case QueryType::Dump:
  case QueryType::List:
  case QueryType::PrintTable:
  case QueryType::Select:
  case QueryType::Count:
  case QueryType::Sum:
  case QueryType::Min:
  case QueryType::Max:
    return QueryKind::Read;

  case QueryType::Load:
  case QueryType::Drop:
  case QueryType::Truncate:
  case QueryType::CopyTable:
  case QueryType::Insert:
  case QueryType::Update:
  case QueryType::Delete:
  case QueryType::Duplicate:
  case QueryType::Add:
  case QueryType::Sub:
  case QueryType::Swap:
    return QueryKind::Write;

  case QueryType::Quit:
  case QueryType::Listen:
  case QueryType::Nop:
  default:
    return QueryKind::Null;
  }
}

// Resolve a queue routing id for a query
// - If query has a concrete table name then return it
// - Otherwise route to control pseudo table "__control__"
inline auto resolveTableId(const Query &query) -> std::string {
  const auto &table = query.table();
  return table.empty() ? controlTableId() : table;
}

// Extract file path for file-related queries (LOAD/DUMP); return empty if none.
inline auto extractFilePath(const Query &query) -> std::string {
  return query.filePath();
}

// Extract the new_table field of CopyTable; return empty if none.
inline auto extractNewTable(const Query &query) -> std::string {
  return query.newTable();
}

// Get the type of queries
inline auto queryType(const Query &query) -> QueryType { return query.type(); }

#endif  // SRC_QUERY_QUERYHELPERS_H_
