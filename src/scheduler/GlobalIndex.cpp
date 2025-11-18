#include "GlobalIndex.h"

#include <cstdint>

#include "../query/QueryPriority.h"
#include "TableQueue.h"

// Push a new Key with incremented version to invalidate older entries.
void GlobalIndex::upsert(TableQueue *tableQ, QueryPriority priorityLevel,
                         std::uint64_t enqueueTick, std::uint64_t headSeq) {
  if (tableQ == nullptr) {
    return;
  }
  auto &ver = latestVersion[tableQ];
  ++ver; // update & record version
  heap.push(Key{.pri = priorityLevel,
                .stamp = enqueueTick,
                .headSeq = headSeq,
                .tbl = tableQ,
                .version = ver});
}

// Discard stale heap entries until a current one surfaces.
auto GlobalIndex::pickBest(TableQueue *&outTableQ) -> bool {
  while (!heap.empty()) {
    const Key &top = heap.top();
    auto versionIt = latestVersion.find(top.tbl);
    if (versionIt == latestVersion.end() || versionIt->second != top.version) {
      heap.pop();
      continue;
    }
    outTableQ = top.tbl;
    heap.pop();
    return true;
  }
  outTableQ = nullptr;
  return false;
}
