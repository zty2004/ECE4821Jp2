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
