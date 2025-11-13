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
  LockManager &operator=(const LockManager &) = delete;

  [[nodiscard]] bool tryLockS(const TableId &id);
  void unlockS(const TableId &id);

  [[nodiscard]] bool tryLockX(const TableId &id);
  void unlockX(const TableId &id);

  void writerIntentBegin(const TableId &id);
  void writerIntentEnd(const TableId &id);

  [[nodiscard]] bool canAdmitShared(const TableId &id) const noexcept;

private:
  struct Entry {
    mutable std::shared_mutex rw;
    std::atomic<int> waiting_writers{0};
  };

  Entry &entry(const TableId &id);

  mutable std::mutex mapMtx_;
  std::unordered_map<TableId, std::unique_ptr<Entry>> map_;
};

#endif  // SRC_RUNTIME_LOCKMANAGER_H_
