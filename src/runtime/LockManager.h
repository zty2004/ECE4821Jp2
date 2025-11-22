//
// LockManager
//

#ifndef SRC_RUNTIME_LOCKMANAGER_H_
#define SRC_RUNTIME_LOCKMANAGER_H_

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

  // Blocking lock (traditional mutex-style)
  void lockS(const TableId &id);
  void unlockS(const TableId &id);

  void lockX(const TableId &id);
  void unlockX(const TableId &id);

private:
  struct Entry {
    mutable std::shared_mutex rw_;
  };

  auto entry(const TableId &id) -> Entry &;
  auto entryConst(const TableId &id) const -> const Entry *;

  mutable std::mutex mapMtx_;
  std::unordered_map<TableId, std::unique_ptr<Entry>> map_;
};

#endif  // SRC_RUNTIME_LOCKMANAGER_H_
