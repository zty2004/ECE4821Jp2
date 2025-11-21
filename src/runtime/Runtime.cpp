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
#include "TaskQueue.h"
#include "WorkerPool.h"

// Factory functions (defined in SimpleTaskQueue.cpp and SimpleWorkerPool.cpp)
extern auto createSimpleTaskQueue() -> std::unique_ptr<TaskQueue>;
extern auto createSimpleWorkerPool(LockManager *lockMgr, TaskQueue *taskQueue)
    -> std::unique_ptr<WorkerPool>;

Runtime::Runtime(std::size_t numThreads)
    : numThreads_(numThreads), lockMgr_(std::make_unique<LockManager>()) {
  // Runtime is only used in multi-threaded mode (numThreads > 1)
  std::cerr << "lemondb: info: multi-threaded mode enabled (" << numThreads
            << " workers)\n";

  taskQueue_ = createSimpleTaskQueue();
  workers_ = createSimpleWorkerPool(lockMgr_.get(), taskQueue_.get());
  workers_->start(numThreads_);
}

Runtime::~Runtime() {
  waitAll();

  // Stop workers before destroying TaskQueue
  if (workers_) {
    workers_->stop();
  }
}

void Runtime::submitQuery(Query::Ptr query, std::size_t orderIndex) {
  ++totalSubmitted_;

  // Create promise and get future
  std::promise<std::unique_ptr<QueryResult>> resultPromise;

  // CRITICAL: Get future BEFORE moving promise
  auto future = resultPromise.get_future();

  // Submit to TaskQueue (promise is moved here)
  taskQueue_->registerTask(std::move(query), std::move(resultPromise));

  // Store future for later retrieval
  {
    std::lock_guard lock(futuresMtx_);
    futures_[orderIndex] = std::move(future);
  }
}

void Runtime::waitAll() {
  // Wait for all futures to complete
  std::lock_guard lock(futuresMtx_);
  for (auto &[idx, future] : futures_) {
    if (future.valid()) {
      future.wait();
    }
  }
}

auto Runtime::getResultsInOrder() -> std::vector<QueryResult::Ptr> {
  std::vector<QueryResult::Ptr> results;
  results.reserve(totalSubmitted_);

  std::lock_guard lock(futuresMtx_);

  // Extract results from futures in submission order
  for (std::size_t i = 1; i <= totalSubmitted_; ++i) {
    auto it = futures_.find(i);
    if (it != futures_.end() && it->second.valid()) {
      try {
        results.push_back(it->second.get());
      } catch (const std::exception &e) {
        auto errorResult = std::make_unique<ErrorMsgResult>(
            "RUNTIME", "", std::string("Exception: ") + e.what());
        results.push_back(std::move(errorResult));
      }
    }
  }

  return results;
}
