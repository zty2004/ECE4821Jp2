#ifndef SRC_SCHEDULER_GLOBALINDEX_H_
#define SRC_SCHEDULER_GLOBALINDEX_H_

#include <cstdint>
#include <queue>
#include <unordered_map>
#include <vector>

#include "../query/QueryPriority.h"

struct TableQueue;

// GlobalIndex is a cross-table selection structure.
// Ordering: higher priority first; then fairness by enqueueTick with a
// tolerance window kFairnessQuantum; finally fallback to headseq
class GlobalIndex {
public:
  GlobalIndex() = default;
  ~GlobalIndex() = default;
  GlobalIndex(const GlobalIndex &) = delete;
  GlobalIndex &operator=(const GlobalIndex &) = delete; // NOLINT
  GlobalIndex(GlobalIndex &&) noexcept = delete;
  GlobalIndex &operator=(GlobalIndex &&) noexcept = delete; // NOLINT

  // Fairness constants (quantization)
  static constexpr std::uint64_t kFairnessQuantum = 64; // can be modified

  void upsert(TableQueue *tableQ, QueryPriority priorityLevel,
              std::uint64_t enqueueTick, std::uint64_t headSeq);
  auto pickBest(TableQueue *&outTableQ) -> bool;

  [[nodiscard]] auto empty() const -> bool { return heap.empty(); }
  [[nodiscard]] auto size() const -> std::size_t { return heap.size(); }

private:
  struct Key {
    QueryPriority pri{};      // priority level of representative key
    std::uint64_t stamp{0};   // enqueueTick
    std::uint64_t headSeq{0}; // sequence index
    TableQueue *tbl{nullptr}; // table queue
    std::uint64_t version{0}; // monotonic to invalidate stale entries
  };
  struct KeyCmp {
    auto operator()(const Key &leftKey, const Key &rightKey) const -> bool {
      if (leftKey.pri != rightKey.pri) {
        return leftKey.pri > rightKey.pri; // higher priority first
      }
      const std::uint64_t diff = (leftKey.stamp > rightKey.stamp)
                                     ? (leftKey.stamp - rightKey.stamp)
                                     : (rightKey.stamp - leftKey.stamp);
      if (diff >= kFairnessQuantum) {
        return leftKey.stamp > rightKey.stamp; // balance work according to time
      }
      return leftKey.headSeq > rightKey.headSeq; // smaller headSeq first
    }
  };
  std::priority_queue<Key, std::vector<Key>, KeyCmp> heap;
  std::unordered_map<TableQueue *, std::uint64_t> latestVersion;
};

#endif // SRC_SCHEDULER_GLOBALINDEX_H_
