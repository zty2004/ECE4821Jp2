#ifndef SRC_SCHEDULER_TABLEQUEUE_H_
#define SRC_SCHEDULER_TABLEQUEUE_H_

#include <cstdint>
#include <deque>

#include "./ScheduledItem.h"

struct TableQueue {
  bool registered = false;       // LOAD or not
  std::uint64_t registerSeq = 0; // seq of LOAD
  std::deque<ScheduledItem> queue;
};

#endif // SR{}C_SCHEDULER_TABLEQUEUE_H_
