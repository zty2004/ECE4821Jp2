//
// TaskQueue fetch logic implementation
//

#include "TaskQueue.h"

#include <atomic>
#include <memory>
#include <string>
#include <utility>

#include "../query/Query.h"
#include "ScheduledItem.h"
#include "TableQueue.h"

void TaskQueue::getFetched(std::unique_ptr<ScheduledItem> &loadCand,
                           ScheduledItem *&tableCand, TableQueue *&tableCandQ) {
  if (!loadQueue.empty() && !loadBlocked) {
    if (!barriers.empty() && loadQueue.front()->seq > barriers.front().seq) {
      loadBlocked = true;
    } else {
      loadCand = std::move(loadQueue.front());
      loadQueue.pop_front();
    }
  }
  TableQueue *picked = nullptr;
  while (globalIndex.pickBest(picked)) {
    if (picked == nullptr || picked->queue.empty()) {
      continue;
    }
    ScheduledItem &head = picked->queue.front();
    if (!barriers.empty() && head.seq > barriers.front().seq) {
      waitingTables.push_back(picked);
      continue;
    }
    tableCand = &head;
    tableCandQ = picked;
    break;
  }
}

auto TaskQueue::fetchBarrier(ExecutableTask &out) -> bool {
  if (!barriers.empty()) {
    auto runningCount = running.load(std::memory_order_relaxed);
    if (runningCount > 0) {
      return false;
    }

    ScheduledItem barrierItem = std::move(barriers.front());
    barriers.pop_front();
    if (barrierItem.type == QueryType::Quit) {
      quitFlag = true;
    }
    buildExecutableFromScheduled(barrierItem, out);
    running.fetch_add(1, std::memory_order_relaxed);
    fetchTick.fetch_add(1, std::memory_order_relaxed);
    for (auto *blocked : waitingTables) {
      if (blocked != nullptr && !blocked->queue.empty()) {
        const ScheduledItem &blockedHead = blocked->queue.front();
        globalIndex.upsert(blocked, blockedHead.priority,
                           fetchTick.load(std::memory_order_relaxed),
                           blockedHead.seq);
      }
    }
    waitingTables.clear();
    loadBlocked = false;
    return true;
  }
  return false;
}

auto TaskQueue::judgeLoadDeps(std::unique_ptr<ScheduledItem> &loadCand)
    -> bool {
  const auto &lDeps = std::get<LoadDeps>(loadCand->depends);
  if (lDeps.fileDependsOn >
      depManager.lastCompletedFor(DependencyManager::DependencyType::File,
                                  lDeps.filePath)) {
    depManager.addWait(DependencyManager::DependencyType::File, lDeps.filePath,
                       std::move(loadCand));
    return true;
  }
  if (lDeps.tableDependsOn >
      depManager.lastCompletedFor(DependencyManager::DependencyType::Table,
                                  loadCand->tableId)) {
    const auto &loadTableId = loadCand->tableId;
    depManager.addWait(DependencyManager::DependencyType::Table, loadTableId,
                       std::move(loadCand));
    return true;
  }
  return false;
}

auto TaskQueue::judgeNormalDeps(ScheduledItem *&tableCand,
                                TableQueue *&tableCandQ) -> bool {
  if (tableCand->type == QueryType::Dump) {
    const auto &dumpDeps = std::get<DumpDeps>(tableCand->depends);
    const std::string filePath = dumpDeps.filePath;
    const std::string tableId = tableCand->tableId;
    if (dumpDeps.fileDependsOn >
        depManager.lastCompletedFor(DependencyManager::DependencyType::File,
                                    filePath)) {
      auto waitingP =
          std::make_unique<ScheduledItem>(std::move(tableCandQ->queue.front()));
      tableCandQ->queue.pop_front();
      depManager.addWait(DependencyManager::DependencyType::File, filePath,
                         std::move(waitingP));
      return true;
    }
    if (dumpDeps.tableDependsOn >
        depManager.lastCompletedFor(DependencyManager::DependencyType::Table,
                                    tableId)) {
      auto waitingP =
          std::make_unique<ScheduledItem>(std::move(tableCandQ->queue.front()));
      tableCandQ->queue.pop_front();
      depManager.addWait(DependencyManager::DependencyType::Table, tableId,
                         std::move(waitingP));
      return true;
    }
  }
  if (tableCand->type == QueryType::Drop) {
    const auto &dropDeps = std::get<DropDeps>(tableCand->depends);
    const std::string tableId = tableCand->tableId;
    auto lastCompleted = depManager.lastCompletedFor(
        DependencyManager::DependencyType::Table, tableId);
    if (dropDeps.tableDependsOn > lastCompleted) {
      auto waitingP =
          std::make_unique<ScheduledItem>(std::move(tableCandQ->queue.front()));
      tableCandQ->queue.pop_front();
      depManager.addWait(DependencyManager::DependencyType::Table, tableId,
                         std::move(waitingP));
      return true;
    }
  }
  if (tableCand->type == QueryType::CopyTable) {
    const auto &copyDeps = std::get<CopyTableDeps>(tableCand->depends);
    const std::string srcTableId = tableCand->tableId;
    const std::string newTable = copyDeps.newTable;
    if (copyDeps.srcTableDependsOn >
        depManager.lastCompletedFor(DependencyManager::DependencyType::Table,
                                    srcTableId)) {
      auto waitingP =
          std::make_unique<ScheduledItem>(std::move(tableCandQ->queue.front()));
      tableCandQ->queue.pop_front();
      depManager.addWait(DependencyManager::DependencyType::Table, srcTableId,
                         std::move(waitingP));
      return true;
    }
    if (copyDeps.dstTableDependsOn >
        depManager.lastCompletedFor(DependencyManager::DependencyType::Table,
                                    newTable)) {
      auto waitingP =
          std::make_unique<ScheduledItem>(std::move(tableCandQ->queue.front()));
      tableCandQ->queue.pop_front();
      depManager.addWait(DependencyManager::DependencyType::Table, newTable,
                         std::move(waitingP));
      return true;
    }
  }
  return false;
}
