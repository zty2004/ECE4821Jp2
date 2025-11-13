//
// LockManager implementation
//

#include "LockManager.h"
#include <atomic>
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

auto LockManager::tryLockS(const TableId &id) -> bool {
  auto &entry_ref = entry(id);
  return entry_ref.rw.try_lock_shared();
}

void LockManager::unlockS(const TableId &id) {
  auto &entry_ref = entry(id);
  entry_ref.rw.unlock_shared();
}

auto LockManager::tryLockX(const TableId &id) -> bool {
  auto &entry_ref = entry(id);
  return entry_ref.rw.try_lock();
}

void LockManager::unlockX(const TableId &id) {
  auto &entry_ref = entry(id);
  entry_ref.rw.unlock();
}

void LockManager::writerIntentBegin(const TableId &id) {
  auto &entry_ref = entry(id);
  entry_ref.waiting_writers.fetch_add(1, std::memory_order_release);
}

void LockManager::writerIntentEnd(const TableId &id) {
  auto &entry_ref = entry(id);
  entry_ref.waiting_writers.fetch_sub(1, std::memory_order_release);
}

auto LockManager::canAdmitShared(const TableId &id) const noexcept -> bool {
  const Entry *found = [&]() -> const Entry * {
    const std::scoped_lock<std::mutex> lock(mapMtx_);
    if (auto iter = map_.find(id); iter != map_.end()) {
      return iter->second.get();
    }
    return nullptr;
  }();

  if (found == nullptr) {
    return true;
  }

  return found->waiting_writers.load(std::memory_order_acquire) == 0;
}
