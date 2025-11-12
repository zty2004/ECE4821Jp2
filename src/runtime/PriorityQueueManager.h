//
// Place holder for PriorityQueueManager
// Simply define some interfaces here
// Should be replaced by Jiangshen's impl
//

#ifndef SRC_RUNTIME_PRIORITYQUEUEMANAGER_H_
#define SRC_RUNTIME_PRIORITYQUEUEMANAGER_H_

#include "QueryTask.h"
#include <concepts>
#include <optional>

template <class PQ>
concept TablePriorityQueue = requires(PQ pq, QueryTask t) {
  { pq.push(std::move(t)) } -> std::same_as<void>;
  { pq.tryPop(t) } -> std::same_as<bool>;
  { pq.empty() } -> std::same_as<bool>;
  { pq.size() } -> std::convertible_to<std::size_t>;
};

struct PickResult {
  bool ok;
  QueryTask task;

  PickResult() : ok(false), task() {}
  explicit PickResult(QueryTask t) : ok(true), task(std::move(t)) {}
};

class PriorityQueueManager {
public:
  virtual ~PriorityQueueManager() = default;

  [[nodiscard]] virtual PickResult tryPickNext() = 0;

  virtual void requeueBack(QueryTask t) = 0;
  virtual void requeueFront(QueryTask t) = 0;
};

#endif // SRC_RUNTIME_PRIORITYQUEUEMANAGER_H_
