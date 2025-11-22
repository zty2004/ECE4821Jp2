//
// TaskQueue dependency management implementation
//

#include "TaskQueue.h"

#include <atomic>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "../db/Database.h"
#include "ScheduledItem.h"

void TaskQueue::applyUpdateDeps(const ScheduledItem &item) {
  std::vector<std::unique_ptr<ScheduledItem>> readyFileItems;
  std::vector<std::unique_ptr<ScheduledItem>> readyTableItems;
  updateDependencyRecords(item, readyFileItems, readyTableItems);
  for (auto &&readyItem : readyFileItems) {
    updateReadyFiles(readyItem);
  }
  for (auto &&readyItem : readyTableItems) {
    updateReadyTables(readyItem);
  }
}

void TaskQueue::updateDependencyRecords(
    const ScheduledItem &item,
    std::vector<std::unique_ptr<ScheduledItem>> &readyFileItems,
    std::vector<std::unique_ptr<ScheduledItem>> &readyTableItems) {
  switch (item.type) {
  case QueryType::Load: {
    const auto &loadDeps = std::get<LoadDeps>(item.depends);
    depManager.notifyCompleted(DependencyManager::DependencyType::File,
                               loadDeps.filePath, item.seq, readyFileItems);
    if (!loadDeps.pendingTable.empty()) {
      for (const auto &ptableId : loadDeps.pendingTable) {
        depManager.notifyCompleted(DependencyManager::DependencyType::Table,
                                   ptableId, item.seq, readyTableItems);
      }
    } else {
      depManager.notifyCompleted(DependencyManager::DependencyType::Table,
                                 item.tableId, item.seq, readyTableItems);
    }
    break;
  }
  case QueryType::Dump: {
    const auto &dumpDeps = std::get<DumpDeps>(item.depends);
    depManager.notifyCompleted(DependencyManager::DependencyType::File,
                               dumpDeps.filePath, item.seq, readyFileItems);
    depManager.notifyCompleted(DependencyManager::DependencyType::Table,
                               item.tableId, item.seq, readyTableItems);
    break;
  }
  case QueryType::Drop: {
    depManager.notifyCompleted(DependencyManager::DependencyType::Table,
                               item.tableId, item.seq, readyTableItems);
    auto tblIt = tables.find(item.tableId);
    if (tblIt != tables.end() && tblIt->second) {
      tblIt->second->registered = false;
    }
    break;
  }
  case QueryType::CopyTable: {
    const auto &copyDeps = std::get<CopyTableDeps>(item.depends);
    depManager.notifyCompleted(DependencyManager::DependencyType::Table,
                               item.tableId, item.seq, readyTableItems);
    depManager.notifyCompleted(DependencyManager::DependencyType::Table,
                               copyDeps.newTable, item.seq, readyTableItems);
    break;
  }
  default:
    if (!item.tableId.empty()) {
      depManager.notifyCompleted(DependencyManager::DependencyType::Table,
                                 item.tableId, item.seq, readyTableItems);
    }
    break;
  }
}

void TaskQueue::updateReadyFiles(std::unique_ptr<ScheduledItem> &readyItem) {
  if (readyItem->type == QueryType::Load) {
    const auto &rlDeps = std::get<LoadDeps>(readyItem->depends);
    if (!rlDeps.pendingTable.empty()) {
      auto &database = Database::getInstance();
      readyItem->tableId = database.getFileTableName(rlDeps.filePath);
    } else if (rlDeps.tableDependsOn >
               depManager.lastCompletedFor(
                   DependencyManager::DependencyType::Table,
                   readyItem->tableId)) {
      const auto &readyTableId = readyItem->tableId;
      depManager.addWait(DependencyManager::DependencyType::Table, readyTableId,
                         std::move(readyItem));
      return;
    }
    loadQueue.emplace_front(std::move(readyItem));
    return;
  }
  auto &pendingTablePtr = tables[readyItem->tableId];
  if (!pendingTablePtr) {
    pendingTablePtr = std::make_unique<TableQueue>();
  }
  auto &pendingTable = *pendingTablePtr;
  pendingTable.queue.push_front(std::move(*readyItem));
  readyItem.reset();
  const auto &head = pendingTable.queue.front();
  globalIndex.upsert(pendingTablePtr.get(), head.priority,
                     fetchTick.load(std::memory_order_relaxed), head.seq);
}

void TaskQueue::updateReadyTables(std::unique_ptr<ScheduledItem> &readyItem) {
  if (readyItem->type == QueryType::Load) {
    loadQueue.emplace_front(std::move(readyItem));
    return;
  }

  if (readyItem->type == QueryType::CopyTable) {
    const auto &rcDeps = std::get<CopyTableDeps>(readyItem->depends);
    const auto &srcTableId = readyItem->tableId;
    if (rcDeps.srcTableDependsOn >
        depManager.lastCompletedFor(DependencyManager::DependencyType::Table,
                                    srcTableId)) {
      depManager.addWait(DependencyManager::DependencyType::Table, srcTableId,
                         std::move(readyItem));
      return;
    }
    if (rcDeps.dstTableDependsOn >
        depManager.lastCompletedFor(DependencyManager::DependencyType::Table,
                                    rcDeps.newTable)) {
      depManager.addWait(DependencyManager::DependencyType::Table,
                         rcDeps.newTable, std::move(readyItem));
      return;
    }
  }
  auto &pendingTablePtr = tables[readyItem->tableId];
  if (!pendingTablePtr) {
    pendingTablePtr = std::make_unique<TableQueue>();
  }
  auto &pendingTable = *pendingTablePtr;
  pendingTable.queue.push_front(std::move(*readyItem));
  readyItem.reset();
  const auto &head = pendingTable.queue.front();
  globalIndex.upsert(pendingTablePtr.get(), head.priority,
                     fetchTick.load(std::memory_order_relaxed), head.seq);
}
