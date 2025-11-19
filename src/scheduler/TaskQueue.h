#ifndef SRC_SCHEDULER_TASKQUEUE_H_
#define SRC_SCHEDULER_TASKQUEUE_H_

#include <cstdint>
#include <deque>
#include <future>
#include <memory>
#include <string>

#include "FileDependencyManager.h"
#include "ScheduledItem.h"

class Query;
class QueryResult;

// TaskQueue public interface
class TaskQueue {
public:
  TaskQueue();
  ~TaskQueue();
  TaskQueue(const TaskQueue &) = delete;
  TaskQueue &operator=(const TaskQueue &) = delete; // NOLINT
  TaskQueue(TaskQueue &&) noexcept = delete;
  TaskQueue &operator=(TaskQueue &&) noexcept = delete; // NOLINT

  // Register a task
  auto registerTask(std::unique_ptr<Query> query)
      -> std::future<std::unique_ptr<QueryResult>>;

  // Fetch next executable task, Returns false if no task is ready
  auto fetchNext(ScheduledItem &out) -> bool;

  // Called after LOAD finished
  // Marks tasks with seq < registerSeq as dropped, then upsert
  void completeLoad(const std::string &tableId, std::uint64_t loadSeq);

  // File operation serialization forwarding hooks
  void beginFileOp(const std::string &filePath, std::uint64_t seq);
  void endFileOp(const std::string &filePath, std::uint64_t seq);

private:
  // Data member
  std::uint64_t nextSeq = 0;
  std::uint64_t fetchTick = 0;

  // TODO: Map of tableId -> TableQueue
  // TODO: Consider std::unordered_map<std::string, TableQueue>
  // TODO: GlobalIndex

  // loadQueue: FIFO of ready LOAD items
  std::deque<FileDependencyManager::LoadNode> loadQueue;

  // FDM instance for per-file dependency management
  FileDependencyManager fileDeps;
};

#endif // SRC_SCHEDULER_TASKQUEUE_H_
