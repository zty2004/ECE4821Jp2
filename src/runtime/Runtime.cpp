//
// Runtime implementation
//

#include "Runtime.h"

#include <cstddef>
#include <future>
#include <memory>
#include <mutex>
#include <string>
#include <utility>
#include <vector>

#include "../query/Query.h"
#include "../query/QueryResult.h"
#include "LockManager.h"

// Forward declarations - to be implemented
class TaskQueue;
class WorkerPool;

Runtime::Runtime(std::size_t numThreads)
    : lockMgr_(std::make_unique<LockManager>()),
      taskQueue_(nullptr),  // TODO: Initialize with TaskQueue implementation
      workers_(nullptr)     // TODO: Initialize with WorkerPool implementation
{
  (void)numThreads;  // Will be used when WorkerPool is implemented

  // TODO: Initialize TaskQueue and WorkerPool when implementations are
  // available taskQueue_ = std::make_unique<TaskQueue>(); workers_ =
  // std::make_unique<WorkerPool>(lockMgr_.get(), taskQueue_.get());
  // workers_->start(numThreads);
  //
  // Note: Runtime creates promise and passes it to TaskQueue::registerTask()
  // TaskQueue stores the promise in ScheduledItem/ExecutableTask
  // Workers call promise.set_value() after execution
}

Runtime::~Runtime() { waitAll(); }

void Runtime::submitQuery(Query::Ptr query, std::size_t orderIndex) {
  // Create promise in Runtime
  std::promise<std::unique_ptr<QueryResult>> resultPromise;

  // Get future before passing promise to TaskQueue
  auto future = resultPromise.get_future();

  // Register task with TaskQueue, passing promise
  // TaskQueue::registerTask will take ownership of the promise
  // taskQueue_->registerTask(std::move(query), std::move(resultPromise));

  // Save future for later retrieval
  {
    std::lock_guard lock(futuresMtx_);
    futures_[orderIndex] = std::move(future);
    ++totalSubmitted_;
  }
}

void Runtime::waitAll() {
  // TaskQueue doesn't have explicit shutdown
  // Workers will keep polling until all tasks are complete

  // TODO: Wait for all workers to complete their tasks
  // workers_->stop();
}

auto Runtime::getResultsInOrder() -> std::vector<QueryResult::Ptr> {
  std::vector<QueryResult::Ptr> results;

  std::lock_guard lock(futuresMtx_);
  results.reserve(totalSubmitted_);

  // Retrieve results in order by calling future.get()
  // This will block until each task completes
  // future.get() returns std::unique_ptr<QueryResult>
  for (auto &[orderIndex, future] : futures_) {
    try {
      // future.get() returns std::unique_ptr<QueryResult>, which is
      // QueryResult::Ptr
      results.push_back(future.get());  // Blocking wait for result
    } catch (const std::exception &e) {
      // If execution failed, create error result
      auto errorResult = std::make_unique<ErrorMsgResult>(
          "RUNTIME", "", std::string("Exception: ") + e.what());
      results.push_back(std::move(errorResult));
    }
  }

  return results;
}
