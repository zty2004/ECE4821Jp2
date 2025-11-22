//
// Threadpool
// When the threadpool has empty slot, it asks scheduler for tasks. This class
// focues on thread management and synchronization.
//

#include "Threadpool.h"
#include <memory>
#include <utility>
#include <vector>

#include "../query/QueryHelpers.h"

Threadpool::Threadpool(std::size_t numThreads, LockManager &lm, TaskQueue &tq)
    : thread_count_(numThreads), lock_manager_(lm), task_queue_(tq) {
  threads_.reserve(numThreads);
  for (std::size_t i = 0; i < numThreads; ++i) {
#ifdef __cpp_lib_jthread
    threads_.emplace_back(
        [this](std::stop_token st) { this->worker_loop(std::move(st)); });
#else
    threads_.emplace_back([this] { this->worker_loop(); });
#endif
  }
}

Threadpool::~Threadpool() {
#ifdef __cpp_lib_jthread
  // jthread automatically requests stop and joins in destructor
#else
  stop_flag_.store(true);
  for (auto &t : threads_) {
    if (t.joinable()) {
      t.join();
    }
  }
#endif
}

/*
[[maybe_unused]] auto Threadpool::get_threadpool_size() const -> size_t {
  return thread_count_;
}
*/

Threadpool::WriteGuard::WriteGuard(LockManager &lkm, TableId index)
    : lm_(lkm), id_(std::move(index)) {
  lm_.lockX(id_);
}

Threadpool::WriteGuard::~WriteGuard() { lm_.unlockX(id_); }

Threadpool::ReadGuard::ReadGuard(LockManager &lkm, TableId index)
    : lm_(lkm), id_(std::move(index)) {
  lm_.lockS(id_);
}

Threadpool::ReadGuard::~ReadGuard() { lm_.unlockS(id_); }

#ifdef __cpp_lib_jthread
void Threadpool::worker_loop(std::stop_token st_) {
  while (!st_.stop_requested()) {
    work();
  }
}
#else
void Threadpool::worker_loop() {
  while (!stop_flag_.load(std::memory_order_acquire)) {
    work();
  }
}
#endif

void Threadpool::work() {
  ExecutableTask task;
  bool has_task = false;
  {
    std::unique_lock<std::mutex> lock(local_mutex_);
    if (local_queue_.empty()) {
      lock.unlock();

      std::vector<ExecutableTask> batch;
      batch.reserve(FETCH_BATCH_SIZE);

      int fetchCount = 0;
      for (size_t i = 0; i < FETCH_BATCH_SIZE; ++i) {
        ExecutableTask tmp;
        if (task_queue_.fetchNext(tmp)) {
          batch.push_back(std::move(tmp));
          fetchCount++;
        } else {
          break;
        }
      }

      if (fetchCount > 0) {
        lock.lock();
        for (auto &tmp : batch) {
          local_queue_.emplace(std::move(tmp));
        }
        // Keep lock held to check and extract task
      } else {
        // Reacquire lock to safely check queue
        lock.lock();
      }
    } else {
      // lock is already held from line 70
    }

    // At this point, lock is always held
    if (!local_queue_.empty()) {
      task = std::move(local_queue_.front());
      local_queue_.pop();
      has_task = true;
    }
    // lock released when going out of scope
  }

  if (has_task) {
    executeTask(task);
  } else {
    // Use shorter sleep when idle to reduce latency
    std::this_thread::sleep_for(std::chrono::microseconds(100));
  }
}

void Threadpool::executeTask(ExecutableTask &task) {
  // If query is nullptr (e.g., execOverride-only tasks), execute without lock
  if (!task.query) {
    executeNull(task);
    return;
  }

  const TableId tid = resolveTableId(*task.query);
  const QueryKind kind = getQueryKind(task.type);

  try {
    if (kind == QueryKind::Write) {
      WriteGuard guard(lock_manager_, tid);
      executeWrite(task);
    } else if (kind == QueryKind::Read) {
      ReadGuard guard(lock_manager_, tid);
      executeRead(task);
    } else {
      executeNull(task);
    }
  } catch (...) {
    // Log error or handle exception
  }
}

void Threadpool::executeWrite(ExecutableTask &task) {
  run_logic(task, "WRITE");
}

void Threadpool::executeRead(ExecutableTask &task) { run_logic(task, "READ"); }

void Threadpool::executeNull(ExecutableTask &task) { run_logic(task, "NULL"); }

void Threadpool::run_logic(ExecutableTask &task, const char * /*type*/) {
  try {
    std::unique_ptr<QueryResult> res;
    if (task.execOverride) {
      // Use custom execution function if provided
      res = task.execOverride();
    } else if (task.query) {
      // Execute the actual query
      res = task.query->execute();
    } else {
      // No query to execute, create a null result
      res = std::make_unique<NullQueryResult>();
    }
    task.promise.set_value(std::move(res));
  } catch (...) {
    try {
      task.promise.set_exception(std::current_exception());
    } catch (...) {
      // Promise already satisfied, ignore
    }
  }

  // Always call onCompleted callback if present, even if execution failed
  // Protect against exceptions in callback
  if (task.onCompleted) {
    try {
      task.onCompleted();
    } catch (...) {
      // Callback should not throw, but protect against it anyway
      // Log or handle the error if needed
    }
  }
}
