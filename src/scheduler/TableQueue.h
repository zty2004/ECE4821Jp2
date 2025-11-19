#ifndef SRC_SCHEDULER_TABLEQUEUE_H_
#define SRC_SCHEDULER_TABLEQUEUE_H_

#include <cstdint>
#include <deque>

#include "./ScheduledItem.h"

struct TableQueue {
  bool registered = false;       // LOAD or not
  std::uint64_t registerSeq = 0; // seq of LOAD
  std::deque<ScheduledItem> queue;

  [[nodiscard]] auto size() const -> std::size_t { return queue.size(); }
  [[nodiscard]] auto empty() const -> bool { return queue.empty(); }
};

#endif // SRC_SCHEDULER_TABLEQUEUE_H_
