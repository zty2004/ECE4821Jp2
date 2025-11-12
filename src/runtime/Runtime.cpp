//
// Runtime implementation
//

#include "Runtime.h"
#include <chrono>
#include <thread>

Runtime::Runtime(std::size_t maxThreads)
    : lockMgr_(std::make_unique<LockManager>()),
      pqMgr_(std::make_unique<SimplePQManager>()) {
  (void)maxThreads; // TODO: Pass to PopUpExecutor when configurable

  auto cb = [this](std::size_t idx, QueryResult::Ptr res) {
    resultCallback(idx, std::move(res));
  };

  executor_ = std::make_unique<PopUpExecutor>(*lockMgr_, *pqMgr_, cb);
}

Runtime::~Runtime() { waitAll(); }

OpKind Runtime::determineOpKind(Query &query) {
  std::string typeName = query.toString();

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

std::string Runtime::extractTableName(const Query &query) {
  return query.getTargetTable();
}

void Runtime::resultCallback(std::size_t orderIndex, QueryResult::Ptr result) {
  std::scoped_lock lk(resultMtx_);
  results_[orderIndex] = std::move(result);
}

void Runtime::submitQuery(Query::Ptr query, std::size_t orderIndex) {
  std::string table = extractTableName(*query);
  OpKind kind = determineOpKind(*query);

  QueryTask task(table, kind, std::move(query), orderIndex);

  {
    std::scoped_lock lk(resultMtx_);
    totalSubmitted_++;
  }

  executor_->submit(std::move(task));
}

void Runtime::waitAll() {
  while (true) {
    std::size_t completed;
    std::size_t total;
    {
      std::scoped_lock lk(resultMtx_);
      completed = results_.size();
      total = totalSubmitted_;
    }

    if (completed >= total && total > 0) {
      break;
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }

  executor_->stop();
}

std::vector<QueryResult::Ptr> Runtime::getResultsInOrder() {
  std::scoped_lock lk(resultMtx_);
  std::vector<QueryResult::Ptr> ordered;
  ordered.reserve(results_.size());

  for (auto &[idx, result] : results_) {
    ordered.push_back(std::move(result));
  }

  return ordered;
}
