//
// WorkerPool - Placeholder Interface
//

#ifndef SRC_RUNTIME_WORKERPOOL_H_
#define SRC_RUNTIME_WORKERPOOL_H_

#include <cstddef>

// Abstract interface for managing worker threads
// Workers continuously fetch tasks from PriorityQueueManager and execute them
class WorkerPool {
public:
  virtual ~WorkerPool() = default;

  // Start the specified number of worker threads
  // Input: numThreads (number of threads to create)
  // Output: void
  // Behavior: Create numThreads worker threads, each running workerLoop()
  virtual void start(std::size_t numThreads) = 0;

  // Stop all worker threads
  // Input: none
  // Output: void
  // Behavior: Wait for all worker threads to finish their current tasks
  //           and exit gracefully
  virtual void stop() = 0;

  // Worker thread loop (implementation-specific, typically private)
  // Input: none
  // Output: void
  // Behavior for each iteration:
  //   1. Call pqMgr_->waitAndPop() to get a task (blocks if empty)
  //   2. If nullopt returned, exit the loop (shutdown signal)
  //   3. Acquire lock based on task.kind:
  //      - OpKind::Write: lockMgr_->lockX(task.tableName)
  //      - OpKind::Read:  lockMgr_->lockS(task.tableName)
  //   4. Execute: result = task.query->execute()
  //   5. Release lock:
  //      - OpKind::Write: lockMgr_->unlockX(task.tableName)
  //      - OpKind::Read:  lockMgr_->unlockS(task.tableName)
  //   6. Set result: task.resultPromise.set_value(std::move(result))
  //   7. If exception occurs:
  //   task.resultPromise.set_exception(std::current_exception())
  //   8. Loop back to step 1
  //
  // Note: The lock must be released BEFORE setting the promise to avoid
  //       holding the lock while the main thread processes the result
};

#endif  // SRC_RUNTIME_WORKERPOOL_H_
