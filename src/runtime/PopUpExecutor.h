//
// PopUpExecutor
//

#ifndef SRC_RUNTIME_POPUPEXECUTOR_H_
#define SRC_RUNTIME_POPUPEXECUTOR_H_

#include <atomic>
#include <condition_variable>
#include <cstddef>
#include <deque>
#include <functional>
#include <mutex>
#include <thread>
#include <utility>
#include <vector>

#include "LockManager.h"
#include "PriorityQueueManager.h"
#include "QueryTask.h"

class PopUpExecutor {
public:
  using ResultCallback =
      std::function<void(std::size_t orderIndex, QueryResult::Ptr result)>;

  explicit PopUpExecutor(LockManager &lm, PriorityQueueManager &pqm,
                         ResultCallback cb) noexcept
      : lm_(lm), pqm_(pqm), resultCb_(std::move(cb)) {}

  ~PopUpExecutor() { stop(); }

  PopUpExecutor(const PopUpExecutor &) = delete;
  auto operator=(const PopUpExecutor &) -> PopUpExecutor & = delete;

  PopUpExecutor(PopUpExecutor &&) = delete;
  auto operator=(PopUpExecutor &&) -> PopUpExecutor & = delete;

  void submit(QueryTask task);
  void stop();

  [[nodiscard]] [[maybe_unused]] auto activeCount() const noexcept
      -> std::size_t {
    return active_.load(std::memory_order_relaxed);
  }

private:
  static constexpr std::size_t kMaxThreads = 8;

  LockManager &lm_;
  PriorityQueueManager &pqm_;
  ResultCallback resultCb_;

  std::mutex qMtx_;
  std::condition_variable qCv_;
  std::deque<QueryTask> ready_;

  std::vector<std::thread> threads_;
  std::atomic<std::size_t> active_{0};
  std::atomic<bool> stopping_{false};

  auto tryPop(QueryTask &out) -> bool;
  void maybeGrow();
  void workerLoop();
  void runOne(QueryTask task);
  void executeRead(QueryTask &task);
  void executeWrite(QueryTask &task);
};

#endif  // SRC_RUNTIME_POPUPEXECUTOR_H_
