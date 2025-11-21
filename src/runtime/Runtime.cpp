//
// Runtime implementation
//

#include "Runtime.h"

#include <cstddef>
#include <future>
#include <iostream>
#include <memory>
#include <mutex>
#include <string>
#include <utility>
#include <vector>

#include "../query/Query.h"
#include "../query/QueryHelpers.h"
#include "../query/QueryResult.h"
#include "../scheduler/TaskQueue.h"
#include "LockManager.h"
#include "Threadpool.h"

Runtime::Runtime(std::size_t numThreads)
    : lockMgr_(std::make_unique<LockManager>()),
      taskQueue_(std::make_unique<TaskQueue>()),
      threadpool_(
          std::make_unique<Threadpool>(numThreads, *lockMgr_, *taskQueue_)) {
  // Runtime is only used in multi-threaded mode (numThreads > 1)
  std::cerr << "lemondb: info: multi-threaded mode enabled (" << numThreads
            << " workers)\n";
}

Runtime::~Runtime() {
  waitAll();
  // Threadpool will be automatically destroyed (jthread auto-joins)
}

void Runtime::submitQuery(Query::Ptr query, std::size_t orderIndex) {
  ++totalSubmitted_;

  // Get query type before moving query
  QueryType qtype = queryType(*query);

  // Prepare ParsedQuery for TaskQueue
  ParsedQuery pq;
  pq.seq = orderIndex;
  pq.tableName = resolveTableId(*query);
  pq.type = qtype;
  pq.priority = classifyPriority(qtype);
  pq.query = std::move(query);
  pq.promise =
      std::promise<std::unique_ptr<QueryResult>>();  // Runtime creates promise

  // TaskQueue will get_future() from promise before moving it
  auto future = taskQueue_->registerTask(std::move(pq));

  // Store future for later retrieval
  {
    std::lock_guard lock(futuresMtx_);
    futures_[orderIndex] = std::move(future);
  }
}

void Runtime::waitAll() {
  // Wait for all futures to complete
  std::lock_guard lock(futuresMtx_);
  for (auto &[idx, future] : futures_) {
    if (future.valid()) {
      future.wait();
    }
  }
}

auto Runtime::getResultsInOrder() -> std::vector<QueryResult::Ptr> {
  std::vector<QueryResult::Ptr> results;
  results.reserve(totalSubmitted_);

  std::lock_guard lock(futuresMtx_);

  // Extract results from futures in submission order
  for (std::size_t i = 1; i <= totalSubmitted_; ++i) {
    auto it = futures_.find(i);
    if (it != futures_.end() && it->second.valid()) {
      try {
        results.push_back(it->second.get());
      } catch (const std::exception &e) {
        auto errorResult = std::make_unique<ErrorMsgResult>(
            "RUNTIME", "", std::string("Exception: ") + e.what());
        results.push_back(std::move(errorResult));
      }
    }
  }

  return results;
}
