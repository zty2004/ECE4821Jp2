#include "FileDependencyManager.h"

#include <cstdint>
#include <string>
#include <utility>
#include <vector>

// Register a scheduled operation for filePath with global seq
// Returns the previous scheduled seq as dependency seq
auto FileDependencyManager::markScheduled(const std::string &filePath,
                                          std::uint64_t seq) -> std::uint64_t {
  auto &cur = lastScheduled[filePath];
  auto prev = cur;
  if (seq > cur) {
    cur = seq;
  }
  return prev;
}

// Record a waiting LOAD node for its target file
void FileDependencyManager::addLoadWait(LoadNode &&node) {
  waiting[node.filePath].push_back(std::move(node));
}

// Notify that file operation with seq completed
// Update the completed record
// Return the LoadNodes that became ready
auto FileDependencyManager::notifyFileCompleted(
    const std::string &filePath, std::uint64_t seq) -> std::vector<LoadNode> {
  auto &completed = lastCompleted[filePath];
  if (seq > completed) {
    completed = seq;
  }

  std::vector<LoadNode> ready;
  auto iter = waiting.find(filePath);
  if (iter == waiting.end()) {
    return ready;
  }

  auto &queue = iter->second;
  while (!queue.empty() && completed >= queue.front().dependsOn) {
    ready.emplace_back(std::move(queue.front()));
    queue.pop_front();
  }

  if (queue.empty()) {
    waiting.erase(iter);
  }
  return ready;
}

// Return last completed seq for a file
auto FileDependencyManager::lastCompletedFor(const std::string &filePath) const
    -> std::uint64_t {
  auto found = lastCompleted.find(filePath);
  if (found == lastCompleted.end()) {
    return 0;
  }
  return found->second;
}
