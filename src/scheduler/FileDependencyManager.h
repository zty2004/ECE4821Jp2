#ifndef SRC_SCHEDULER_FILEDEPENDENCYMANAGER_H_
#define SRC_SCHEDULER_FILEDEPENDENCYMANAGER_H_

#include <cstdint>
#include <deque>
#include <string>
#include <unordered_map>
#include <vector>

#include "ScheduledItem.h"

// FileDependencyManager manages per-file LOAD/DUMP dependencies
class FileDependencyManager {
public:
  FileDependencyManager() = default;
  ~FileDependencyManager() = default;
  FileDependencyManager(const FileDependencyManager &) = delete;
  FileDependencyManager &
  operator=(const FileDependencyManager &) = delete; // NOLINT
  FileDependencyManager(FileDependencyManager &&) noexcept = delete;
  FileDependencyManager &
  operator=(FileDependencyManager &&) noexcept = delete; // NOLINT

  struct LoadNode {
    std::string filePath;
    std::uint64_t dependsOn;
    ScheduledItem item; // move-owned
    LoadNode() : filePath(), dependsOn(0), item() {}
  };

  // Register a scheduled operation for filePath with global seq
  // Returns the previous scheduled seq as dependency seq
  auto markScheduled(const std::string &filePath,
                     std::uint64_t seq) -> std::uint64_t;

  // Record a waiting LOAD node for its target file
  void addLoadWait(LoadNode &&node);

  // Notify that file operation with seq completed
  // Update the completed record
  // Return the LoadNodes that became ready
  auto notifyFileCompleted(const std::string &filePath,
                           std::uint64_t seq) -> std::vector<LoadNode>;

  // Return last completed seq for a file
  [[nodiscard]] auto
  lastCompletedFor(const std::string &filePath) const -> std::uint64_t;

private:
  std::unordered_map<std::string, std::uint64_t> lastScheduled;
  std::unordered_map<std::string, std::uint64_t> lastCompleted;
  std::unordered_map<std::string, std::deque<LoadNode>> waiting;
};

#endif // SRC_SCHEDULER_FILEDEPENDENCYMANAGER_H_
