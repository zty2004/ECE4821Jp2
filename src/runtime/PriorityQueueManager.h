//
// Place holder for PriorityQueueManager
// Simply define some interfaces here
// Should be replaced by Jiangshen's impl
//

#ifndef SRC_RUNTIME_PRIORITYQUEUEMANAGER_H_
#define SRC_RUNTIME_PRIORITYQUEUEMANAGER_H_

#include <concepts>
#include <optional>
#include <utility>

#include "QueryTask.h"

template <class PQ>
concept TablePriorityQueue = requires(PQ pq, QueryTask task) {
  { pq.push(std::move(task)) } -> std::same_as<void>;
  { pq.tryPop(task) } -> std::same_as<bool>;
  { pq.empty() } -> std::same_as<bool>;
  { pq.size() } -> std::convertible_to<std::size_t>;
};

struct PickResult {
  // NOLINTBEGIN(misc-non-private-member-variables-in-classes)
  bool ok;
  QueryTask task;
  // NOLINTEND(misc-non-private-member-variables-in-classes)

  PickResult() : ok(false) {}
  explicit PickResult(QueryTask taskArg) : ok(true), task(std::move(taskArg)) {}
};

class PriorityQueueManager {
public:
  PriorityQueueManager() = default;
  virtual ~PriorityQueueManager() = default;

  PriorityQueueManager(const PriorityQueueManager &) = delete;
  auto operator=(const PriorityQueueManager &)
      -> PriorityQueueManager & = delete;

  PriorityQueueManager(PriorityQueueManager &&) = delete;
  auto operator=(PriorityQueueManager &&) -> PriorityQueueManager & = delete;

  [[nodiscard]] virtual auto tryPickNext() -> PickResult = 0;

  virtual void requeueBack(QueryTask task) = 0;
  virtual void requeueFront(QueryTask task) = 0;
};

#endif  // SRC_RUNTIME_PRIORITYQUEUEMANAGER_H_
