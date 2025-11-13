//
// LockManager
//

#ifndef SRC_RUNTIME_LOCKMANAGER_H_
#define SRC_RUNTIME_LOCKMANAGER_H_

#include <atomic>
#include <memory>
#include <mutex>
#include <shared_mutex>
#include <string>
#include <unordered_map>

using TableId = std::string;

class LockManager {
public:
  LockManager() = default;
  ~LockManager() = default;

  LockManager(const LockManager &) = delete;
  auto operator=(const LockManager &) -> LockManager & = delete;

  LockManager(LockManager &&) = delete;
  auto operator=(LockManager &&) -> LockManager & = delete;

  [[nodiscard]] auto tryLockS(const TableId &id) -> bool;
  void unlockS(const TableId &id);

  [[nodiscard]] auto tryLockX(const TableId &id) -> bool;
  void unlockX(const TableId &id);

  void writerIntentBegin(const TableId &id);
  void writerIntentEnd(const TableId &id);

  [[nodiscard]] auto canAdmitShared(const TableId &id) const noexcept -> bool;

private:
  struct Entry {
    // NOLINTBEGIN(misc-non-private-member-variables-in-classes,codequality-no-public-member-variables)
    mutable std::shared_mutex rw;
    std::atomic<int> waiting_writers{0};
    // NOLINTEND(misc-non-private-member-variables-in-classes,codequality-no-public-member-variables)
  };

  auto entry(const TableId &id) -> Entry &;

  mutable std::mutex mapMtx_;
  std::unordered_map<TableId, std::unique_ptr<Entry>> map_;
};

#endif  // SRC_RUNTIME_LOCKMANAGER_H_
