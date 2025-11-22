#include "DependencyManager.h"

#include <cstdint>
#include <string>

#include "../query/QueryHelpers.h"
#include "ScheduledItem.h"

void DependencyManager::markScheduled(ScheduledItem &item, QueryType tag) {
  auto seq = item.seq;
  const std::string &tableId = item.tableId;

  if (item.type == QueryType::Load) {
    std::vector<std::string> pendingTables;
    auto filePath = extractFilePath(*item.query);
    auto prevFile = markScheduledFile(filePath, seq, tag);
    auto prevFileTag = prevFile.first;
    auto prevFileSeq = prevFile.second;

    auto completedIter = lastCompletedFile.find(filePath);
    std::uint64_t const fileCompleted =
        completedIter == lastCompletedFile.end() ? 0 : completedIter->second;

    item.droppedFlag = (prevFileTag == QueryType::Load &&
                        lastScheduledTable[tableId].first != QueryType::Drop);

    bool const pending =
        (prevFileTag == QueryType::Dump && fileCompleted < prevFileSeq);

    if (pending) {
      // Pending LOAD
      // Apply Nop to any tables currently marked Drop to enforce one-hop wait
      for (auto &entry : lastScheduledTable) {
        if (entry.second.first == QueryType::Drop) {
          entry.second.first = QueryType::Nop;
          entry.second.second = seq;
          pendingTables.emplace_back(entry.first);
        }
      }
      // Record only file dependency, tableDependsOn left 0 (to be resolved
      // after file complete)
      item.depends = LoadDeps(prevFileSeq, 0, filePath, pendingTables);
      return;
    }

    auto prevTable = markScheduledTable(tableId, seq, tag);
    auto prevTableSeq = prevTable.second;
    item.depends = LoadDeps(prevFileSeq, prevTableSeq, filePath, pendingTables);
    return;
  }

  if (item.type == QueryType::Dump) {
    auto filePath = extractFilePath(*item.query);
    auto res = markScheduledFile(filePath, seq, tag);
    auto fileDependsOn = res.second;
    item.depends = DumpDeps(fileDependsOn, filePath);
    return;
  }

  if (item.type == QueryType::Drop) {
    auto res = markScheduledTable(tableId, seq, tag);
    auto tableDependsOn = res.second;
    item.depends = DropDeps(tableDependsOn);
    return;
  }

  if (item.type == QueryType::CopyTable) {
    auto res = markScheduledTable(tableId, seq, tag);
    auto srcTableDependsOn = res.second;
    auto newTableId = extractNewTable(*item.query);
    res = markScheduledTable(newTableId, seq, tag);
    auto dstTableDependsOn = res.second;
    item.depends =
        CopyTableDeps(srcTableDependsOn, dstTableDependsOn, newTableId);
    return;
  }
}

auto DependencyManager::markScheduledFile(const std::string &filePath,
                                          std::uint64_t seq, QueryType tag)
    -> std::pair<QueryType, std::uint64_t> {
  auto ret = lastScheduledFile[filePath];
  auto &fileEntry = lastScheduledFile[filePath];
  if (seq > fileEntry.second) {
    fileEntry.first = tag;
    fileEntry.second = seq;
  }
  return ret;
}

auto DependencyManager::markScheduledTable(const std::string &tableId,
                                           std::uint64_t seq, QueryType tag)
    -> std::pair<QueryType, std::uint64_t> {
  auto ret = lastScheduledTable[tableId];
  auto &tableEntry = lastScheduledTable[tableId];
  if (seq > tableEntry.second) {
    tableEntry.first = tag;
    tableEntry.second = seq;
  }
  return ret;
}

void DependencyManager::addWait(const DependencyType &type,
                                const std::string &key,
                                std::unique_ptr<ScheduledItem> item) {
  auto &waitingMap = type == DependencyType::File ? waitingFile : waitingTable;
  waitingMap[key].push(item);
}

void DependencyManager::notifyCompleted(
    const DependencyType &type, const std::string &key, std::uint64_t seq,
    std::vector<std::unique_ptr<ScheduledItem>> &ready) {
  auto &completedSeq = type == DependencyType::File ? lastCompletedFile[key]
                                                    : lastCompletedTable[key];
  if (seq < completedSeq) {
    return;
  }
  completedSeq = seq;
  auto &waitingMap = type == DependencyType::File ? waitingFile : waitingTable;
  auto waitingIter = waitingMap.find(key);
  if (waitingIter == waitingMap.end()) {
    return;
  }
  auto &heap = waitingIter->second;
  while (!heap.empty()) {
    ScheduledItem const &waitItem = *heap.top();
    if (waitItem.type == QueryType::Load &&
        std::get<LoadDeps>(waitItem.depends).fileDependsOn < completedSeq) {
      break;
    }
    ready.emplace_back(
        std::move(const_cast<std::unique_ptr<ScheduledItem> &>(heap.top())));
    heap.pop();
  }
  if (heap.empty()) {
    waitingMap.erase(waitingIter);
  }
}

auto DependencyManager::lastCompletedFor(
    const DependencyType &type, const std::string &key) const -> std::uint64_t {
  const auto &lastCompletedMap =
      type == DependencyType::File ? lastCompletedFile : lastCompletedTable;
  auto iter = lastCompletedMap.find(key);
  return iter == lastCompletedMap.end() ? 0 : iter->second;
}
