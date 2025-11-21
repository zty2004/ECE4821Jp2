//
// Threadpool
// When the threadpool has empty slot, it asks scheduler for tasks. This class
// focues on thread management and synchronization.
//

#ifndef SRC_RUNTIME_THREADPOOL_H_
#define SRC_RUNTIME_THREADPOOL_H_

#include "../scheduler/TaskQueue.h"
#include "LockManager.h"
#include <atomic>
#include <cstddef>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>

#ifdef __cpp_lib_jthread
#include <stop_token>
#endif

class Threadpool {

public:
  static constexpr size_t FETCH_BATCH_SIZE = 16;  // Fetch 16 tasks each time

  Threadpool(std::size_t numThreads, LockManager &lm, TaskQueue &tq);

  ~Threadpool();

  Threadpool(const Threadpool &) = delete;
  Threadpool(Threadpool &&) = delete;
  auto operator=(const Threadpool &) -> Threadpool & = delete;
  auto operator=(Threadpool &&) -> Threadpool & = delete;

  [[nodiscard]] auto get_threadpool_size() const -> size_t;

private:
  std::size_t thread_count_;
  LockManager &lock_manager_;
  TaskQueue &task_queue_;

  std::queue<ExecutableTask> local_queue_;
  std::mutex local_mutex_;  // protect local_queue_

#ifdef __cpp_lib_jthread
  std::vector<std::jthread> threads_;
#else
  std::vector<std::thread> threads_;
  std::atomic<bool> stop_flag_{false};
#endif

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
  void worker_loop(std::stop_token st);
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
