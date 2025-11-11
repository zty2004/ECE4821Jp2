#ifndef SRC_SCHEDULER_GLOBALINDEX_H_
#define SRC_SCHEDULER_GLOBALINDEX_H_

#include <cstdint>
#include <memory>

#include "../query/QueryPriority.h"

struct TableQueue;

class GlobalIndex {
public:
  GlobalIndex();
  ~GlobalIndex();
  GlobalIndex(const GlobalIndex &) = delete;
  GlobalIndex &operator=(const GlobalIndex &) = delete; // NOLINT
  GlobalIndex(GlobalIndex &&) noexcept = delete;
  GlobalIndex &operator=(GlobalIndex &&) noexcept = delete; // NOLINT

  // Fairness constants (quantization)
  static constexpr std::uint64_t kFairnessQuantum = 64; // can be modified

  // Insert or update the key for a table
  // priorityLevel: cross-table priority of the table head
  // enqueueTick: per-table fairness stamp
  // means older headSeq: the global sequence of the current table head
  void upsert(TableQueue *tableQ, QueryPriority priorityLevel,
              std::uint64_t enqueueTick, std::uint64_t headSeq); // NOLINT

  // Atomically pick and remove the best table id, return false if empty
  bool pickBest(TableQueue *&outTableQ); // NOLINT

private:
  struct Impl;
  std::unique_ptr<Impl> impl_;
};

#endif // SRC_SCHEDULER_GLOBALINDEX_H_
