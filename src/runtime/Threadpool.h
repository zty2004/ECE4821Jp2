//
// Threadpool
// Accept one task from Manager, distribute it to a thread. This class focues on
// thread management and synchronization.
//

#ifndef SRC_RUNTIME_THREADPOOL_H_
#define SRC_RUNTIME_THREADPOOL_H_

#define __cpp_lib_jthread

#include "../query/Query.h"
#include "../query/QueryResult.h"
#include "LockManager.h"
#include <array>
#include <cstddef>
#include <future>
#include <queue>
#include <utility>

// just to erase red cross
struct ExecutableTask {
  std::uint64_t seq = 0;
  std::unique_ptr<Query> query;
  std::promise<std::unique_ptr<QueryResult>> promise;
  std::function<std::unique_ptr<QueryResult>()>
      execOverride;                   // preset function
  std::function<void()> onCompleted;  // callback closure

  ExecutableTask() = default;
  ~ExecutableTask() = default;
  ExecutableTask(ExecutableTask &&) noexcept = default;
  ExecutableTask &operator=(ExecutableTask &&) noexcept = default;  // NOLINT
  ExecutableTask(const ExecutableTask &) = delete;
  ExecutableTask &operator=(const ExecutableTask &) = delete;  // NOLINT
};

class TaskQueue {
public:
  auto fetchNext(ExecutableTask &out) -> bool;
};

#ifdef __cpp_lib_jthread
/**
 * @brief The type of threads to use. In C++20 and later we use `std::jthread`.
 */
#include <stop_token>
#include <thread>
using thread_t = std::jthread;
#else
/**
 * @brief The type of threads to use. In C++17 we use`std::thread`.
 */
#include <thread>
using thread_t = std::thread;
#endif

template <std::size_t PoolSize> class Threadpool {

public:
  static constexpr size_t FETCH_BATCH_SIZE = 16;  // Fetch 16 tasks each time

  Threadpool(LockManager &lm, TaskQueue &tq)
      : lock_manager_(lm), task_queue_(tq) {
    for (std::size_t i = 0; i < PoolSize; ++i) {
#ifdef __cpp_lib_jthread
      threads[i] =
          thread_t([this](std::stop_token st) { this->worker_loop(st); });
#else
      threads[i] = thread_t([this] { this->worker_loop(); });
#endif
    }
  }

  ~Threadpool() {
    stop_flag_.store(true);
#ifndef __cpp_lib_jthread
    for (auto &t : threads) {
      if (t.joinable()) {
        t.join();
      }
    }
#endif
  }

  Threadpool(const Threadpool &) = delete;
  Threadpool(Threadpool &&) = delete;
  auto operator=(const Threadpool &) -> Threadpool & = delete;
  auto operator=(Threadpool &&) -> Threadpool & = delete;

  [[nodiscard]] auto get_threadpool_size() const -> size_t {
    return thread_count;
  }

private:
  static constexpr std::size_t thread_count = PoolSize;
  struct PooledTask {
    ExecutableTask task;
    int attempts = 0;

    PooledTask() = default;
    PooledTask(ExecutableTask &&t) : task(std::move(t)), attempts(0) {}
  };
  LockManager &lock_manager_;
  TaskQueue &task_queue_;

  std::queue<ExecutableTask> local_queue_;
  std::mutex local_mutex_;  // protect local_queue_

  std::array<thread_t, PoolSize> threads;
  std::atomic<bool> stop_flag_{false};

#ifdef __cpp_lib_jthread
  void worker_loop(const std::stop_token &st) {
    while (!st.stop_requested()) {
      work();
    }
  }
#else
  void worker_loop() {
    while (!stop_flag_.load(std::memory_order_acquire)) {
      work();
    }
  }
#endif

  void work() {
    PooledTask current_item;
    bool has_task = false;
    {
      std::unique_lock<std::mutex> lock(local_mutex_);
      if (local_queue_.empty()) {
        lock.unlock();

        std::vector<ExecutableTask> batch;
        batch.reserve(FETCH_BATCH_SIZE);

        for (size_t i = 0; i < FETCH_BATCH_SIZE; ++i) {
          ExecutableTask tmp;
          if (task_queue_.fetchNext(tmp)) {
            batch.push_back(std::move(tmp));
          } else {
            break;
          }
        }

        lock.lock();
        for (auto &tmp : batch) {
          local_queue_.emplace(std::move(tmp));
        }
      }

      if (!local_queue_.empty()) {
        current_item = std::move(local_queue_.front());
        local_queue_.pop();
        has_task = true;
      }
    }

    if (has_task) {
      if (!tryExecuteTask(current_item)) {
        std::unique_lock<std::mutex> lock(local_mutex_);
        local_queue_.push(std::move(current_item));
        lock.unlock();

        std::this_thread::yield();
      }
    } else {
      std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
  }

  auto tryExecuteTask(PooledTask &item) -> bool {
    ExecutableTask &task = item.task;

    if (!task.query) {
      executeGeneral(task);
      return true;
    }

    const TableId tid = task.query->getTargetTable();
    // const OpKind kind = task.query->kind;

    if (kind == OpKind::Write) {
      if (item.attempts == 0) {
        lock_manager_.writerIntentBegin(tid);
      }

      if (lock_manager_.tryLockX(tid)) {
        executeWrite(task);
        lock_manager_.unlockX(tid);
        lock_manager_.writerIntentEnd(tid);
        return true;
      }

      item.attempts++;
      return false;
    }

    if (kind == OpKind::Read) {
      if (!lock_manager_.canAdmitShared(tid)) {
        item.attempts++;
        return false;
      }

      if (lock_manager_.tryLockS(tid)) {
        executeRead(task);
        lock_manager_.unlockS(tid);
        return true;
      }

      item.attempts++;
      return false;
    }

    return true;
  }

  void executeWrite(ExecutableTask &task) { run_logic(task, "WRITE"); }
  void executeRead(ExecutableTask &task) { run_logic(task, "READ"); }
  void executeGeneral(ExecutableTask &task) { run_logic(task, "GENERAL"); }

  void run_logic(ExecutableTask &task) {
    try {
      std::unique_ptr<QueryResult> res;
      if (task.execOverride) {
        res = task.execOverride();
      }
      task.promise.set_value(std::move(res));
    } catch (...) {
      try {
        task.promise.set_exception(std::current_exception());
      } catch (...) {
      }
    }
    if (task.onCompleted) {
      task.onCompleted();
    }
  }
};

#endif  // SRC_RUNTIME_THREADPOOL_H_