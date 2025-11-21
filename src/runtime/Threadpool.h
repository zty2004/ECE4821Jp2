//
// Threadpool
// When the threadpool has empty slot, it asks scheduler for tasks. This class
// focues on thread management and synchronization.
//

#ifndef SRC_RUNTIME_THREADPOOL_H_
#define SRC_RUNTIME_THREADPOOL_H_

#define __cpp_lib_jthread

#include "../query/QueryHelpers.h"
#include "../query/QueryResult.h"
#include "../scheduler/TaskQueue.h"
#include "LockManager.h"
#include <array>
#include <cstddef>
#include <future>
#include <queue>
#include <utility>

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

  Threadpool(LockManager &lm, TaskQueue &tq);

  ~Threadpool();

  Threadpool(const Threadpool &) = delete;
  Threadpool(Threadpool &&) = delete;
  auto operator=(const Threadpool &) -> Threadpool & = delete;
  auto operator=(Threadpool &&) -> Threadpool & = delete;

  [[nodiscard]] auto get_threadpool_size() const -> size_t;

private:
  static constexpr std::size_t thread_count = PoolSize;
  LockManager &lock_manager_;
  TaskQueue &task_queue_;

  std::queue<ExecutableTask> local_queue_;
  std::mutex local_mutex_;  // protect local_queue_

  std::array<thread_t, PoolSize> threads;
  std::atomic<bool> stop_flag_{false};

  class WriteGuard {
  public:
    WriteGuard(LockManager &lkm, TableId index);

    ~WriteGuard();

    WriteGuard(const WriteGuard &) = delete;
    auto operator=(const WriteGuard &) -> WriteGuard & = delete;
    WriteGuard(WriteGuard &&) = delete;
    auto operator=(WriteGuard &&) -> WriteGuard & = delete;

  private:
    LockManager &lm_;
    TableId id_;
  };

  class ReadGuard {
  public:
    ReadGuard(LockManager &lkm, TableId index);

    ~ReadGuard();

    ReadGuard(const ReadGuard &) = delete;
    auto operator=(const ReadGuard &) -> ReadGuard & = delete;
    ReadGuard(ReadGuard &&) = delete;
    auto operator=(ReadGuard &&) -> ReadGuard & = delete;

  private:
    LockManager &lm_;
    TableId id_;
  };

#ifdef __cpp_lib_jthread
  void worker_loop(const std::stop_token &st);
#else
  void worker_loop();
#endif

  void work() {
    ExecutableTask task;
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
        task = std::move(local_queue_.front());
        local_queue_.pop();
        has_task = true;
      }
    }

    if (has_task) {
      executeTask(task);
    } else {
      std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
  }

  void executeTask(ExecutableTask &task) {
    if (!task.query) {
      executeNull(task);
      return;
    }

    const TableId tid = resolveTableId(*task.query);
    const QueryKind kind = getQueryKind(queryType(*task.query));

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
    }
  }

  void executeWrite(ExecutableTask &task) { run_logic(task, "WRITE"); }
  void executeRead(ExecutableTask &task) { run_logic(task, "READ"); }
  void executeNull(ExecutableTask &task) { run_logic(task, "NULL"); }

  void run_logic(ExecutableTask &task, const char *type) {
    try {
      std::unique_ptr<QueryResult> res;
      if (task.execOverride) {
        res = task.execOverride();
      } else {
        res = std::make_unique<QueryResult>(std::string(type) + " Result");
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