#include "DependencyManager.h"

#include <cstdint>
#include <string>

#include "../query/QueryHelpers.h"
#include "ScheduledItem.h"

void DependencyManager::markScheduled(ScheduledItem &item, QueryType tag) {
  auto seq = item.seq;
  const std::string &tableId = item.tableId;

  if (item.type == QueryType::Load) {
    auto filePath = extractFilePath(*item.query);
    auto res = markScheduledFile(filePath, seq, tag);
    auto fileDependsOn = res.second;
    res = markScheduledTable(tableId, seq, tag);
    auto tableDependsOn = res.second;
    item.depends = LoadDeps(fileDependsOn, tableDependsOn, filePath);
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
    item.depends = CopyTableDeps(srcTableDependsOn, dstTableDependsOn);
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
                                const std::string &key, ScheduledItem *item) {
  auto &waitingMap = type == DependencyType::File ? waitingFile : waitingTable;
  waitingMap[key].push(item);
}

auto DependencyManager::notifyCompleted(
    const DependencyType &type, const std::string &key,
    std::uint64_t seq) -> std::vector<ScheduledItem *> {
  std::vector<ScheduledItem *> ready;
  auto &completedSeq = type == DependencyType::File ? lastCompletedFile[key]
                                                    : lastCompletedTable[key];
  if (seq < completedSeq) {
    return ready;
  }
  completedSeq = seq;
  auto &waitingMap = type == DependencyType::File ? waitingFile : waitingTable;
  auto waitingIter = waitingMap.find(key);
  if (waitingIter == waitingMap.end()) {
    return ready;
  }
  auto &heap = waitingIter->second;
  if (!heap.empty()) {
    ScheduledItem const &waitItem = *heap.top();
    if (waitItem.type == QueryType::Load &&
        std::get<LoadDeps>(waitItem.depends).fileDependsOn < completedSeq) {
      return ready;
    }
    ready.push_back(heap.top());
    heap.pop();
  }
  if (heap.empty()) {
    waitingMap.erase(waitingIter);
  }
  return ready;
}

auto DependencyManager::lastCompletedFor(
    const DependencyType &type, const std::string &key) const -> std::uint64_t {
  auto &lastCompleted = type == DependencyType::File ? lastCompletedFile[key]
                                                     : lastCompletedTable[key];
  auto iter = lastCompletedFile.find(key);
  return iter == lastCompletedFile.end() ? 0 : iter->second;
}
