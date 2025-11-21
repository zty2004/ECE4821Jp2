#ifndef SRC_SCHEDULER_DEPENDENCYMANAGER_H_
#define SRC_SCHEDULER_DEPENDENCYMANAGER_H_

#include <cstdint>
#include <queue>
#include <string>
#include <unordered_map>
#include <vector>

#include "../query/Query.h"
#include "ScheduledItem.h"

class DependencyManager {
private:
  struct Cmp {
    auto operator()(const ScheduledItem *itema,
                    const ScheduledItem *itemb) -> bool {
      return itema->seq > itemb->seq;
    }
  };

  using WaitingHeap =
      std::priority_queue<ScheduledItem *, std::vector<ScheduledItem *>, Cmp>;

  enum class DependencyType : std::uint8_t {
    File,
    Table,
  };

public:
  DependencyManager() = default;
  ~DependencyManager() = default;
  DependencyManager(const DependencyManager &) = delete;
  DependencyManager &operator=(const DependencyManager &) = delete; // NOLINT
  DependencyManager(DependencyManager &&) noexcept = delete;
  DependencyManager &
  operator=(DependencyManager &&) noexcept = delete; // NOLINT

  void markScheduled(ScheduledItem &item, QueryType tag);

  void addWait(const DependencyType &type, const std::string &key,
               ScheduledItem *item);

  [[nodiscard]] auto
  notifyCompleted(const DependencyType &type, const std::string &key,
                  std::uint64_t seq) -> std::vector<ScheduledItem *>;

  [[nodiscard]] auto
  lastCompletedFor(const DependencyType &type,
                   const std::string &key) const -> std::uint64_t;

private:
  std::unordered_map<std::string, std::pair<QueryType, std::uint64_t>>
      lastScheduledFile;
  std::unordered_map<std::string, std::uint64_t> lastCompletedFile;
  std::unordered_map<std::string, std::pair<QueryType, std::uint64_t>>
      lastScheduledTable;
  std::unordered_map<std::string, std::uint64_t> lastCompletedTable;

  std::unordered_map<std::string, WaitingHeap> waitingFile;
  std::unordered_map<std::string, WaitingHeap> waitingTable;

  auto markScheduledFile(const std::string &filePath, std::uint64_t seq,
                         QueryType tag) -> std::pair<QueryType, std::uint64_t>;

  auto markScheduledTable(const std::string &tableId, std::uint64_t seq,
                          QueryType tag) -> std::pair<QueryType, std::uint64_t>;
};

#endif // SRC_SCHEDULER_DEPENDENCYMANAGER_H_
