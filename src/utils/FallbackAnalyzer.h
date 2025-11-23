//
// Fallback Analyzer - Decides whether to fallback to single-threaded mode
//

#ifndef SRC_UTILS_FALLBACKANALYZER_H_
#define SRC_UTILS_FALLBACKANALYZER_H_

#include <cstddef>
#include <memory>
#include <vector>

#include "../query/Query.h"

// Thresholds for fallback decision
struct FallbackThresholds {
  size_t minQueries = 1000;  // Minimum query count for multi-threading
  size_t minTables = 3;      // Minimum table count for multi-threading
  // size_t minThreads = 4;  // Disabled: Minimum thread count for
  // multi-threading
};

// Workload statistics
struct WorkloadStats {
  size_t queryCount = 0;
  size_t tableCount = 0;
};

// Estimate complexity of a single query
auto estimateQueryComplexity(const Query &query) -> size_t;

// Analyze workload from query list
auto analyzeWorkload(const std::vector<Query::Ptr> &queries) -> WorkloadStats;

// Decide whether to fallback to single-threaded mode
auto shouldFallback(size_t threadCount, const WorkloadStats &stats,
                    const FallbackThresholds &thresholds) -> bool;

#endif  // SRC_UTILS_FALLBACKANALYZER_H_
