#ifndef SRC_SCHEDULER_TASKQUEUE_H_
#define SRC_SCHEDULER_TASKQUEUE_H_

#include <atomic>
#include <cstdint>
#include <deque>
#include <functional>
#include <future>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>

#include "FileDependencyManager.h"
#include "GlobalIndex.h"
#include "ScheduledItem.h"
#include "TableQueue.h"

// Executable task given to workers
struct ExecutableTask {
  std::uint64_t seq = 0;
  std::unique_ptr<Query> query;
  std::promise<std::unique_ptr<QueryResult>> promise;
  std::function<std::unique_ptr<QueryResult>()> execOverride; // preset function
  std::function<void()> onCompleted; // callback closure

  ExecutableTask() = default;
  ~ExecutableTask() = default;
  ExecutableTask(ExecutableTask &&) noexcept = default;
  ExecutableTask &operator=(ExecutableTask &&) noexcept = default; // NOLINT
  ExecutableTask(const ExecutableTask &) = delete;
  ExecutableTask &operator=(const ExecutableTask &) = delete; // NOLINT
};

class Query;
class QueryResult;

// TaskQueue public interface
class TaskQueue {
public:
  TaskQueue() = default;
  ~TaskQueue() = default;
  TaskQueue(const TaskQueue &) = delete;
  TaskQueue &operator=(const TaskQueue &) = delete; // NOLINT
  TaskQueue(TaskQueue &&) noexcept = delete;
  TaskQueue &operator=(TaskQueue &&) noexcept = delete; // NOLINT

  // Register a task
  auto registerTask(std::unique_ptr<Query> query)
      -> std::future<std::unique_ptr<QueryResult>>;

  // Fetch next executable task, Returns false if no task is ready
  auto fetchNext(ExecutableTask &out) -> bool;

  // Called after LOAD finished
  // Marks tasks with seq < registerSeq as dropped, then upsert
  void completeLoad(const std::string &tableId, std::uint64_t loadSeq);

private:
  // Data member
  std::mutex mu;
  std::atomic<std::uint64_t> fetchTick{0};
  std::atomic<std::uint64_t> submitted{0};
  std::atomic<std::uint64_t> running{0};
  std::atomic<std::uint64_t> completed{0};

  struct Stats {
    std::uint64_t fetchTick{0};
    std::uint64_t submitted{0};
    std::uint64_t running{0};
    std::uint64_t completed{0};
  };

  [[nodiscard]] auto stats() const -> Stats {
    return Stats{.fetchTick = fetchTick.load(std::memory_order_relaxed),
                 .submitted = submitted.load(std::memory_order_relaxed),
                 .running = running.load(std::memory_order_relaxed),
                 .completed = completed.load(std::memory_order_relaxed)};
  }

  // Map of tableId -> TableQueue
  std::unordered_map<std::string, TableQueue> tables;

  // Cross-table selection structure
  GlobalIndex globalIndex;

  // loadQueue: FIFO of ready LOAD items
  std::deque<ScheduledItem> loadQueue;
  bool loadBlocked = false; // whether LoadQueue blocked by barrier

  // FDM instance for per-file dependency management
  FileDependencyManager fileDeps;

  // Barrier structures for QUIT/LIST queries
  std::deque<ScheduledItem> barriers;
  std::deque<TableQueue *> waitingTables; // tables blocked by current barrier

  bool quitFlag = false; // whether QUIT is fetched
};

#endif // SRC_SCHEDULER_TASKQUEUE_H_
