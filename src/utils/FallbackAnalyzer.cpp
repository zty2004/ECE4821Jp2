//
// Fallback Analyzer implementation
//

#include "FallbackAnalyzer.h"

#include <cstddef>
#include <memory>
#include <string>
#include <unordered_set>
#include <vector>

#include "../query/Query.h"

// Estimate complexity based on query type
auto estimateQueryComplexity(const Query &query) -> size_t {
  switch (query.type()) {
  // Simple single-row operations
  case QueryType::Insert:
  case QueryType::Select:
  case QueryType::Delete:
  case QueryType::Update:
    return 1;

  // Table scan operations
  case QueryType::Count:
  case QueryType::Sum:
  case QueryType::Min:
  case QueryType::Max:
  case QueryType::Add:
  case QueryType::Sub:
  case QueryType::CopyTable:
    return 2;

  // Multi-row operations
  case QueryType::Duplicate:
  case QueryType::Swap:
    return 2;

  // Table-level operations
  case QueryType::Truncate:
  case QueryType::Drop:
    return 5;

  // File I/O operations (heavy)
  case QueryType::Dump:
  case QueryType::Load:
    return 10;

  // Management commands (no computation)
  case QueryType::Listen:
  case QueryType::List:
  case QueryType::Quit:
  case QueryType::PrintTable:
  case QueryType::Nop:
  default:
    return 0;
  }
}

// Analyze workload from query list
auto analyzeWorkload(const std::vector<Query::Ptr> &queries) -> WorkloadStats {
  WorkloadStats stats;
  std::unordered_set<std::string> uniqueTables;

  stats.queryCount = queries.size();

  for (const auto &query : queries) {
    // Count unique tables
    const std::string &tableId = query->table();
    if (!tableId.empty()) {
      uniqueTables.insert(tableId);
    }
  }

  stats.tableCount = uniqueTables.size();

  return stats;
}

// Decide whether to fallback to single-threaded mode
auto shouldFallback([[maybe_unused]] size_t threadCount,
                    const WorkloadStats &stats,
                    const FallbackThresholds &thresholds) -> bool {
  // Fallback if any dimension is below threshold
  if (stats.queryCount < thresholds.minQueries) {
    return true;
  }

  if (stats.tableCount < thresholds.minTables) {
    return true;
  }

  // Disabled: Thread count check
  // if (threadCount < thresholds.minThreads) {
  //   return true;
  // }

  return false;
}
