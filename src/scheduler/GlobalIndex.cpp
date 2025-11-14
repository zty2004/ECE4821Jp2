#include <cstdint>

#include "../query/QueryPriority.h"
#include "GlobalIndex.h"
#include "TableQueue.h"

// Upsert: push a new Key with incremented version to invalidate older entries.
void GlobalIndex::upsert(TableQueue *tableQ, QueryPriority priorityLevel,
                         std::uint64_t enqueueTick, std::uint64_t headSeq) {
  if (tableQ == nullptr) {
    return;
  }
  auto &ver = latestVersion_[tableQ];
  ++ver; // update & record version
  heap_.push(Key{.pri = priorityLevel,
                 .stamp = enqueueTick,
                 .headSeq = headSeq,
                 .tbl = tableQ,
                 .version = ver});
}

// PickBest: discard stale heap entries until a current one surfaces.
auto GlobalIndex::pickBest(TableQueue *&outTableQ) -> bool {
  while (!heap_.empty()) {
    const Key &top = heap_.top();
    auto versionIt = latestVersion_.find(top.tbl);
    if (versionIt == latestVersion_.end() || versionIt->second != top.version) {
      heap_.pop();
      continue;
    }
    outTableQ = top.tbl;
    heap_.pop();
    return true;
  }
  outTableQ = nullptr;
  return false;
}
