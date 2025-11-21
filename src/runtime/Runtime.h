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
#include "../scheduler/TaskQueue.h"
#include "LockManager.h"
#include "Threadpool.h"

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
  std::unique_ptr<LockManager> lockMgr_;
  std::unique_ptr<TaskQueue> taskQueue_;
  std::unique_ptr<Threadpool> threadpool_;

  std::mutex futuresMtx_;

  // Store futures for all submitted queries
  std::map<std::size_t, std::future<std::unique_ptr<QueryResult>>> futures_;

  std::size_t totalSubmitted_{0};
};

#endif  // SRC_RUNTIME_RUNTIME_H_
