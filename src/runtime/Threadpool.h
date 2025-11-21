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

  void work();

  void executeTask(ExecutableTask &task);

  void executeWrite(ExecutableTask &task);
  void executeRead(ExecutableTask &task);
  void executeNull(ExecutableTask &task);

  void run_logic(ExecutableTask &task, const char *type);
};

#endif  // SRC_RUNTIME_THREADPOOL_H_