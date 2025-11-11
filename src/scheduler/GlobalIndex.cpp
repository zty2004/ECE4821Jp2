#include "GlobalIndex.h"
#include "TableQueue.h"

struct GlobalIndex::Impl {};

GlobalIndex::GlobalIndex() : impl_(new Impl()) {}
GlobalIndex::~GlobalIndex() = default;

void GlobalIndex::upsert(TableQueue *tableQ, QueryPriority priority,
                         std::uint64_t enqueueTick,
                         std::uint64_t headSeq) { // NOLINT
  (void)tableQ;
  (void)priority;
  (void)enqueueTick;
  (void)headSeq;
}

bool GlobalIndex::pickBest(TableQueue *&outTableQ) { // NOLINT
  outTableQ = nullptr;
  return false;
}
