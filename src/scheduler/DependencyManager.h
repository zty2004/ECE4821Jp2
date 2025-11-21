#ifndef SRC_SCHEDULER_DEPENDENCYMANAGER_H_
#define SRC_SCHEDULER_DEPENDENCYMANAGER_H_

#include <cstdint>
#include <deque>
#include <string>
#include <unordered_map>

#include "../query/Query.h"
#include "ScheduledItem.h"

class DependencyManager {
public:
  DependencyManager() = default;
  ~DependencyManager() = default;
  DependencyManager(const DependencyManager &) = delete;
  DependencyManager &operator=(const DependencyManager &) = delete; // NOLINT
  DependencyManager(DependencyManager &&) noexcept = delete;
  DependencyManager &
  operator=(DependencyManager &&) noexcept = delete; // NOLINT

  void markScheduled(ScheduledItem &item, QueryType tag);

  void addFileWait(const std::string &filePath, ScheduledItem *item,
                   std::uint64_t dependsOn);

  void addTableWait(const std::string &tableId, ScheduledItem *item,
                    std::uint64_t dependsOn);

  [[nodiscard]] auto
  notifyFileCompleted(const std::string &filePath,
                      std::uint64_t seq) -> std::vector<ScheduledItem *>;

  [[nodiscard]] auto
  notifyTableCompleted(const std::string &tableId,
                       std::uint64_t seq) -> std::vector<ScheduledItem *>;

  [[nodiscard]] auto
  lastCompletedForFile(const std::string &filePath) const -> std::uint64_t;

  [[nodiscard]] auto
  lastCompletedForTable(const std::string &tableId) const -> std::uint64_t;

private:
  std::unordered_map<std::string, std::pair<QueryType, std::uint64_t>>
      lastScheduledFile;
  std::unordered_map<std::string, std::uint64_t> lastCompletedFile;
  std::unordered_map<std::string, std::pair<QueryType, std::uint64_t>>
      lastScheduledTable;
  std::unordered_map<std::string, std::uint64_t> lastCompletedTable;
  std::unordered_map<std::string, std::deque<ScheduledItem *>> waitingFile;
  std::unordered_map<std::string, std::deque<ScheduledItem *>> waitingTable;

  auto markScheduledFile(const std::string &filePath, std::uint64_t seq,
                         QueryType tag) -> std::pair<QueryType, std::uint64_t>;

  auto markScheduledTable(const std::string &tableId, std::uint64_t seq,
                          QueryType tag) -> std::pair<QueryType, std::uint64_t>;
};

#endif // SRC_SCHEDULER_DEPENDENCYMANAGER_H_
