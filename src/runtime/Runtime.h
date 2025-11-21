//
// Runtime
//

#ifndef SRC_RUNTIME_RUNTIME_H_
#define SRC_RUNTIME_RUNTIME_H_

#include <cstddef>
#include <future>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

#include "../query/Query.h"
#include "../query/QueryResult.h"
#include "LockManager.h"

class Runtime {
public:
  explicit Runtime(std::size_t numThreads);
  ~Runtime();

  Runtime(const Runtime &) = delete;
  auto operator=(const Runtime &) -> Runtime & = delete;
  Runtime(Runtime &&) = delete;
  auto operator=(Runtime &&) -> Runtime & = delete;

  // Submit a query and get a future for its result
  void submitQuery(Query::Ptr query, std::size_t orderIndex);

  // Wait for all submitted queries to complete
  void waitAll();

  // Get all results in submission order
  auto getResultsInOrder() -> std::vector<QueryResult::Ptr>;

private:
  [[nodiscard]] auto isSingleThreadMode() const noexcept -> bool {
    return numThreads_ == 1;
  }

  std::size_t numThreads_;

  std::unique_ptr<LockManager> lockMgr_;
  // TODO: Add TaskQueue and WorkerPool when implemented
  // std::unique_ptr<TaskQueue> taskQueue_;
  // std::unique_ptr<WorkerPool> workers_;

  std::mutex futuresMtx_;

  // Single-threaded mode: store results directly
  std::map<std::size_t, QueryResult::Ptr> results_;

  // Multi-threaded mode: store futures
  std::map<std::size_t, std::future<std::unique_ptr<QueryResult>>> futures_;

  std::size_t totalSubmitted_{0};
};

#endif  // SRC_RUNTIME_RUNTIME_H_
