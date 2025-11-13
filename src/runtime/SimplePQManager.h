//
// SimplePQManager
// Directly use a single deque as a fake priority queue
//

#ifndef SRC_RUNTIME_SIMPLEPQMANAGER_H_
#define SRC_RUNTIME_SIMPLEPQMANAGER_H_

#include "PriorityQueueManager.h"
#include <deque>
#include <mutex>

class SimplePQManager : public PriorityQueueManager {
public:
  SimplePQManager() = default;
  ~SimplePQManager() override = default;

  SimplePQManager(const SimplePQManager &) = delete;
  SimplePQManager &operator=(const SimplePQManager &) = delete;

  [[nodiscard]] PickResult tryPickNext() override {
    std::scoped_lock lk(mtx_);
    if (queue_.empty()) {
      return PickResult();
    }
    QueryTask t = std::move(queue_.front());
    queue_.pop_front();
    return PickResult(std::move(t));
  }

  void requeueBack(QueryTask t) override {
    std::scoped_lock lk(mtx_);
    queue_.push_back(std::move(t));
  }

  void requeueFront(QueryTask t) override {
    std::scoped_lock lk(mtx_);
    queue_.push_front(std::move(t));
  }

  void push(QueryTask t) {
    std::scoped_lock lk(mtx_);
    queue_.push_back(std::move(t));
  }

  [[nodiscard]] std::size_t size() const {
    std::scoped_lock lk(mtx_);
    return queue_.size();
  }

private:
  mutable std::mutex mtx_;
  std::deque<QueryTask> queue_;
};

#endif  // SRC_RUNTIME_SIMPLEPQMANAGER_H_
