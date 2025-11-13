//
// SimplePQManager
// Directly use a single deque as a fake priority queue
//

#ifndef SRC_RUNTIME_SIMPLEPQMANAGER_H_
#define SRC_RUNTIME_SIMPLEPQMANAGER_H_

#include <deque>
#include <mutex>
#include <utility>

#include "PriorityQueueManager.h"

class SimplePQManager : public PriorityQueueManager {
public:
  SimplePQManager() = default;
  ~SimplePQManager() override = default;

  SimplePQManager(const SimplePQManager &) = delete;
  auto operator=(const SimplePQManager &) -> SimplePQManager & = delete;

  SimplePQManager(SimplePQManager &&) = delete;
  auto operator=(SimplePQManager &&) -> SimplePQManager & = delete;

  [[nodiscard]] auto tryPickNext() -> PickResult override {
    const std::scoped_lock lock(mtx_);
    if (queue_.empty()) {
      return {};
    }
    QueryTask task = std::move(queue_.front());
    queue_.pop_front();
    return PickResult(std::move(task));
  }

  void requeueBack(QueryTask task) override {
    const std::scoped_lock lock(mtx_);
    queue_.push_back(std::move(task));
  }

  void requeueFront(QueryTask task) override {
    const std::scoped_lock lock(mtx_);
    queue_.push_front(std::move(task));
  }

  void push(QueryTask task) {
    const std::scoped_lock lock(mtx_);
    queue_.push_back(std::move(task));
  }

  [[nodiscard]] auto size() const -> std::size_t {
    const std::scoped_lock lock(mtx_);
    return queue_.size();
  }

private:
  mutable std::mutex mtx_;
  std::deque<QueryTask> queue_;
};

#endif  // SRC_RUNTIME_SIMPLEPQMANAGER_H_
