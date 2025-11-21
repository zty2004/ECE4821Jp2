//
// Runtime implementation
//

#include "Runtime.h"

#include <cstddef>
#include <future>
#include <iostream>
#include <memory>
#include <mutex>
#include <string>
#include <utility>
#include <vector>

#include "../query/Query.h"
#include "../query/QueryResult.h"
#include "LockManager.h"

Runtime::Runtime(std::size_t numThreads)
    : numThreads_(numThreads), lockMgr_(std::make_unique<LockManager>()) {
  if (!isSingleThreadMode()) {
    std::cerr << "lemondb: warning: multi-threaded mode not yet implemented\n";
    std::cerr << "lemondb: info: falling back to single-threaded mode\n";
    numThreads_ = 1;
  }
}

Runtime::~Runtime() { waitAll(); }

void Runtime::submitQuery(Query::Ptr query, std::size_t orderIndex) {
  // Currently only single-threaded mode is supported
  std::lock_guard lock(futuresMtx_);
  ++totalSubmitted_;

  try {
    results_[orderIndex] = query->execute();
  } catch (const std::exception &e) {
    // If execution failed, create error result
    auto errorResult = std::make_unique<ErrorMsgResult>(
        "RUNTIME", "", std::string("Exception: ") + e.what());
    results_[orderIndex] = std::move(errorResult);
  }
}

void Runtime::waitAll() {
  // Single-threaded mode: all queries already executed
}

auto Runtime::getResultsInOrder() -> std::vector<QueryResult::Ptr> {
  std::vector<QueryResult::Ptr> results;
  results.reserve(totalSubmitted_);

  std::lock_guard lock(futuresMtx_);

  // Extract results in order
  for (std::size_t i = 1; i <= totalSubmitted_; ++i) {
    auto it = results_.find(i);
    if (it != results_.end()) {
      results.push_back(std::move(it->second));
    }
  }

  return results;
}
