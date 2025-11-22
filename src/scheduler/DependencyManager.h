#ifndef SRC_SCHEDULER_DEPENDENCYMANAGER_H_
#define SRC_SCHEDULER_DEPENDENCYMANAGER_H_

#include <cstdint>
#include <memory>
#include <queue>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "../query/Query.h"
#include "ScheduledItem.h"

class DependencyManager {
private:
  struct Cmp {
    auto operator()(const std::unique_ptr<ScheduledItem> &itema,
                    const std::unique_ptr<ScheduledItem> &itemb) const -> bool {
      return itema->seq > itemb->seq;
    }
  };

  using WaitingHeap =
      std::priority_queue<std::unique_ptr<ScheduledItem>,
                          std::vector<std::unique_ptr<ScheduledItem>>, Cmp>;

public:
  DependencyManager() = default;
  ~DependencyManager() = default;
  DependencyManager(const DependencyManager &) = delete;
  auto operator=(const DependencyManager &) -> DependencyManager & = delete;
  DependencyManager(DependencyManager &&) noexcept = delete;
  auto operator=(DependencyManager &&) noexcept -> DependencyManager & = delete;

  enum class DependencyType : std::uint8_t {
    File,
    Table,
  };

  void markScheduled(ScheduledItem &item,  // NOLINT(runtime/references)
                     QueryType tag);

  void addWait(const DependencyType &type, const std::string &key,
               std::unique_ptr<ScheduledItem> &&item);

  void notifyCompleted(const DependencyType &type, const std::string &key,
                       std::uint64_t seq,
                       std::vector<std::unique_ptr<ScheduledItem>>
                           &ready);  // NOLINT(runtime/references)

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

  auto static notifyCompleteBreakHelper(const DependencyType &type,
                                        const uint64_t &completedSeq,
                                        const ScheduledItem &waitItem) -> bool;
};

#endif  // SRC_SCHEDULER_DEPENDENCYMANAGER_H_
