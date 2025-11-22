//
// Threadpool
// When the threadpool has empty slot, it asks scheduler for tasks. This class
// focues on thread management and synchronization.
//

#ifndef SRC_RUNTIME_THREADPOOL_H_
#define SRC_RUNTIME_THREADPOOL_H_

#include <cstddef>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>

#ifdef __cpp_lib_jthread
#include <stop_token>
#endif

#include "../scheduler/TaskQueue.h"
#include "LockManager.h"

class Threadpool {
public:
  static constexpr size_t FETCH_BATCH_SIZE = 16;  // Fetch 16 tasks each time

  Threadpool(std::size_t numThreads,
             LockManager &lm,  // NOLINT(runtime/references)
             TaskQueue &tq);   // NOLINT(runtime/references)

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
    WriteGuard(LockManager &lkm,  // NOLINT(runtime/references)
               TableId index);

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
    ReadGuard(LockManager &lkm,  // NOLINT(runtime/references)
              TableId index);

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

  void executeTask(ExecutableTask &task);  // NOLINT(runtime/references)

  static void executeWrite(ExecutableTask &task);  // NOLINT(runtime/references)
  static void executeRead(ExecutableTask &task);   // NOLINT(runtime/references)
  static void executeNull(ExecutableTask &task);   // NOLINT(runtime/references)

  // void run_logic(ExecutableTask &task, const char *type);
};

#endif  // SRC_RUNTIME_THREADPOOL_H_
