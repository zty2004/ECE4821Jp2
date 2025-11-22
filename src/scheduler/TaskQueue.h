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

#include "../query/Query.h"
#include "DependencyManager.h"
#include "GlobalIndex.h"
#include "ScheduledItem.h"
#include "TableQueue.h"

// Parsed query needed from parser
struct ParsedQuery {
  std::uint64_t seq = 0;  // global sequence of the query
  std::string tableName;  // name of the corresponding table
  QueryType type;
  QueryPriority priority;
  std::unique_ptr<Query> query;
  std::promise<std::unique_ptr<QueryResult>> promise;
};

// Executable task given to workers
struct ExecutableTask {
  std::uint64_t seq = 0;                               // NOLINT
  QueryType type{QueryType::Nop};                      // NOLINT
  std::unique_ptr<Query> query;                        // NOLINT
  std::promise<std::unique_ptr<QueryResult>> promise;  // NOLINT
  std::function<std::unique_ptr<QueryResult>()>
      execOverride;                   // preset function //NOLINT
  std::function<void()> onCompleted;  // callback closure //NOLINT

  ExecutableTask() = default;
  ~ExecutableTask() = default;
  ExecutableTask(ExecutableTask &&) noexcept = default;
  ExecutableTask &operator=(ExecutableTask &&) noexcept = default;  // NOLINT
  ExecutableTask(const ExecutableTask &) = delete;
  ExecutableTask &operator=(const ExecutableTask &) = delete;  // NOLINT
};

class Query;
class QueryResult;

// TaskQueue public interface
class TaskQueue {
public:
  TaskQueue() = default;
  ~TaskQueue() = default;
  TaskQueue(const TaskQueue &) = delete;
  TaskQueue &operator=(const TaskQueue &) = delete;  // NOLINT
  TaskQueue(TaskQueue &&) noexcept = delete;
  TaskQueue &operator=(TaskQueue &&) noexcept = delete;  // NOLINT

  // Register a task
  auto registerTask(ParsedQuery &&parsedQuery)
      -> std::future<std::unique_ptr<QueryResult>>;

  // Mark that all tasks have been registered, ready to fetch
  void setReady();

  // Fetch next executable task, Returns false if no task is ready
  auto fetchNext(ExecutableTask &out) -> bool;

private:
  // Data member
  std::mutex mu;
  std::atomic<std::uint64_t> fetchTick{0};
  std::atomic<std::uint64_t> submitted{0};
  std::atomic<std::uint64_t> running{0};
  std::atomic<std::uint64_t> completed{0};
  std::atomic<bool> readyToFetch_{false};  // Whether all tasks registered

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
  std::unordered_map<std::string, std::unique_ptr<TableQueue>> tables;

  // Cross-table selection structure
  GlobalIndex globalIndex;

  // loadQueue: FIFO of ready LOAD items
  std::deque<std::unique_ptr<ScheduledItem>> loadQueue;
  bool loadBlocked = false;  // whether LoadQueue blocked by barrier

  // DM instance for dependency management
  DependencyManager depManager;

  // Barrier structures for QUIT/LIST queries
  std::deque<ScheduledItem> barriers;
  std::deque<TableQueue *> waitingTables;  // tables blocked by current barrier

  bool quitFlag = false;  // whether QUIT is fetched

  // Internal helper to materialize ExecutableTask from a ScheduledItem
  void buildExecutableFromScheduled(ScheduledItem &src, ExecutableTask &dst);

  // Completion actions
  enum class CompletionAction : std::uint8_t { RegisterTable, UpdateDeps };
  using ActionList = std::vector<CompletionAction>;

  auto classifyActions(const ScheduledItem &item) -> ActionList;
  void applyActions(const ActionList &actions, const ScheduledItem &item);

  void applyRegisterTable(const ScheduledItem &item);
  void applyUpdateDeps(const ScheduledItem &item);

  void registerTableQueue(std::unique_ptr<TableQueue> &tblPtr,
                          const ScheduledItem &item);
  void updateDependencyRecords(
      const ScheduledItem &item,
      std::vector<std::unique_ptr<ScheduledItem>> &readyFileItems,
      std::vector<std::unique_ptr<ScheduledItem>> &readyTableItems);
  void updateReadyFiles(std::unique_ptr<ScheduledItem> &readyItem);
  void updateReadyTables(std::unique_ptr<ScheduledItem> &readyItem);

  // FetchNext helpers
  auto fetchBarrier(ExecutableTask &out) -> bool;
};

#endif  // SRC_SCHEDULER_TASKQUEUE_H_
