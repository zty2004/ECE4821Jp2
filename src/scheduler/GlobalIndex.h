#ifndef SRC_SCHEDULER_GLOBALINDEX_H_
#define SRC_SCHEDULER_GLOBALINDEX_H_

#include <cstdint>
#include <memory>
#include <string>

#include "../query/QueryPriority.h"

class GlobalIndex {
public:
  GlobalIndex();
  ~GlobalIndex();
  GlobalIndex(const GlobalIndex &) = delete;
  GlobalIndex &operator=(const GlobalIndex &) = delete; // NOLINT
  GlobalIndex(GlobalIndex &&) noexcept = delete;
  GlobalIndex &operator=(GlobalIndex &&) noexcept = delete; // NOLINT

  // Insert or update the key for a table.
  void upsert(const std::string &tableId, QueryPriority priorityLevel,
              std::uint64_t agingBucket, std::uint64_t headSeq);

  // Atomically pick and remove the best table id, return false if empty
  bool pickBest(std::string &outTableId);

private:
  struct Impl;
  std::unique_ptr<Impl> impl_;
};

#endif // SRC_SCHEDULER_GLOBALINDEX_H_
