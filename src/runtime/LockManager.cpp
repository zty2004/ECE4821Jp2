//
// LockManager implementation
//

#include "LockManager.h"
#include <atomic>
#include <memory>
#include <mutex>

LockManager::Entry &LockManager::entry(const TableId &id) {
  std::scoped_lock lk(mapMtx_);
  auto it = map_.find(id);
  if (it == map_.end()) {
    it = map_.emplace(id, std::make_unique<Entry>()).first;
  }
  return *it->second;
}

bool LockManager::tryLockS(const TableId &id) {
  auto &e = entry(id);
  return e.rw.try_lock_shared();
}

void LockManager::unlockS(const TableId &id) {
  auto &e = entry(id);
  e.rw.unlock_shared();
}

bool LockManager::tryLockX(const TableId &id) {
  auto &e = entry(id);
  return e.rw.try_lock();
}

void LockManager::unlockX(const TableId &id) {
  auto &e = entry(id);
  e.rw.unlock();
}

void LockManager::writerIntentBegin(const TableId &id) {
  auto &e = entry(id);
  e.waiting_writers.fetch_add(1, std::memory_order_release);
}

void LockManager::writerIntentEnd(const TableId &id) {
  auto &e = entry(id);
  e.waiting_writers.fetch_sub(1, std::memory_order_release);
}

bool LockManager::canAdmitShared(const TableId &id) const noexcept {
  const Entry *found = [&]() -> const Entry * {
    std::scoped_lock lk(mapMtx_);
    if (auto it = map_.find(id); it != map_.end()) {
      return it->second.get();
    }
    return nullptr;
  }();

  if (!found) {
    return true;
  }

  return found->waiting_writers.load(std::memory_order_acquire) == 0;
}
