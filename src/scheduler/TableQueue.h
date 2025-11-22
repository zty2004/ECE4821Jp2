#ifndef SRC_SCHEDULER_TABLEQUEUE_H_
#define SRC_SCHEDULER_TABLEQUEUE_H_

#include <cstddef>
#include <cstdint>
#include <deque>

#include "./ScheduledItem.h"

struct TableQueue {
  bool registered{false};           // LOAD or not //NOLINT
  std::uint64_t registerSeq{0};     // seq of LOAD //NOLINT
  std::deque<ScheduledItem> queue;  // NOLINT

  [[nodiscard]] auto size() const -> std::size_t { return queue.size(); }
  [[nodiscard]] auto empty() const -> bool { return queue.empty(); }
};

#endif  // SRC_SCHEDULER_TABLEQUEUE_H_
