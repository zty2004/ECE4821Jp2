//
// Runtime implementation
//

#include "Runtime.h"

#include <chrono>
#include <cstddef>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <utility>
#include <vector>

#include "../query/Query.h"
#include "../query/QueryResult.h"
#include "LockManager.h"
#include "PopUpExecutor.h"
#include "QueryTask.h"
#include "SimplePQManager.h"

constexpr int kWaitIntervalMs = 10;

Runtime::Runtime(std::size_t maxThreads)
    : lockMgr_(std::make_unique<LockManager>()),
      pqMgr_(std::make_unique<SimplePQManager>()) {
  (void)maxThreads;  // TODO: Pass to PopUpExecutor when configurable

  auto callback = [this](std::size_t idx, QueryResult::Ptr res) {
    resultCallback(idx, std::move(res));
  };

  executor_ = std::make_unique<PopUpExecutor>(*lockMgr_, *pqMgr_, callback);
}

Runtime::~Runtime() { waitAll(); }

auto Runtime::determineOpKind(Query &query) -> OpKind {
  const std::string typeName = query.toString();

  if (typeName.find("SELECT") != std::string::npos ||
      typeName.find("COUNT") != std::string::npos ||
      typeName.find("SUM") != std::string::npos ||
      typeName.find("MIN") != std::string::npos ||
      typeName.find("MAX") != std::string::npos ||
      typeName.find("PRINT") != std::string::npos ||
      typeName.find("DUMP") != std::string::npos ||
      typeName.find("LIST") != std::string::npos) {
    return OpKind::Read;
  }

  return OpKind::Write;
}

auto Runtime::extractTableName(const Query &query) -> std::string {
  return query.getTargetTable();
}

void Runtime::resultCallback(std::size_t orderIndex, QueryResult::Ptr result) {
  const std::scoped_lock<std::mutex> lock(resultMtx_);
  results_[orderIndex] = std::move(result);
}

void Runtime::submitQuery(Query::Ptr query, std::size_t orderIndex) {
  const std::string table = extractTableName(*query);
  const OpKind kind = determineOpKind(*query);

  QueryTask task(table, kind, std::move(query), orderIndex);

  {
    const std::scoped_lock<std::mutex> lock(resultMtx_);
    totalSubmitted_++;
  }

  executor_->submit(std::move(task));
}

void Runtime::waitAll() {
  while (true) {
    std::size_t completed = 0;
    std::size_t total = 0;
    {
      const std::scoped_lock<std::mutex> lock(resultMtx_);
      completed = results_.size();
      total = totalSubmitted_;
    }

    if (completed >= total && total > 0) {
      break;
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(kWaitIntervalMs));
  }

  executor_->stop();
}

auto Runtime::getResultsInOrder() -> std::vector<QueryResult::Ptr> {
  const std::scoped_lock<std::mutex> lock(resultMtx_);
  std::vector<QueryResult::Ptr> ordered;
  ordered.reserve(results_.size());

  for (auto &[idx, result] : results_) {
    ordered.push_back(std::move(result));
  }

  return ordered;
}
