//
// LockManager implementation
//

#include "LockManager.h"
#include <memory>
#include <mutex>

auto LockManager::entry(const TableId &id) -> LockManager::Entry & {
  const std::scoped_lock<std::mutex> lock(mapMtx_);
  auto iter = map_.find(id);
  if (iter == map_.end()) {
    iter = map_.emplace(id, std::make_unique<Entry>()).first;
  }
  return *iter->second;
}

/*
[[maybe_unused]] auto
LockManager::entryConst(const TableId &id) const -> const Entry * {
  const std::scoped_lock<std::mutex> lock(mapMtx_);
  if (auto iter = map_.find(id); iter != map_.end()) {
    return iter->second.get();
  }
  return nullptr;
}
*/

void LockManager::lockS(const TableId &id) {
  auto &entry_ref = entry(id);
  entry_ref.rw_.lock_shared();  // Blocking call
}

void LockManager::unlockS(const TableId &id) {
  auto &entry_ref = entry(id);
  entry_ref.rw_.unlock_shared();
}

void LockManager::lockX(const TableId &id) {
  auto &entry_ref = entry(id);
  entry_ref.rw_.lock();  // Blocking call
}

void LockManager::unlockX(const TableId &id) {
  auto &entry_ref = entry(id);
  entry_ref.rw_.unlock();
}
