//
// LockManager implementation
//

#include "LockManager.h"
#include <atomic>
#include <memory>
#include <mutex>

LockManager::Entry &LockManager::entry(const TableId &id) {
  const std::scoped_lock<std::mutex> lock(mapMtx_);
  auto iter = map_.find(id);
  if (iter == map_.end()) {
    iter = map_.emplace(id, std::make_unique<Entry>()).first;
  }
  return *iter->second;
}

auto LockManager::entryConst(const TableId &id) const -> const Entry * {
  const std::scoped_lock<std::mutex> lock(mapMtx_);
  if (auto iter = map_.find(id); iter != map_.end()) {
    return iter->second.get();
  }
  return nullptr;
}

void LockManager::lockS(const TableId &id) {
  auto &entry_ref = entry(id);
  entry_ref.rw.lock_shared();  // Blocking call
}

void LockManager::unlockS(const TableId &id) {
  auto &entry_ref = entry(id);
  entry_ref.rw.unlock_shared();
}

void LockManager::lockX(const TableId &id) {
  auto &entry_ref = entry(id);
  entry_ref.rw.lock();  // Blocking call
}

void LockManager::unlockX(const TableId &id) {
  auto &entry_ref = entry(id);
  entry_ref.rw.unlock();
}
