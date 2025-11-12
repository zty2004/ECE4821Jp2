//
// Runtime implementation
//

#include "Runtime.h"
#include <chrono>
#include <thread>

Runtime::Runtime(std::size_t maxThreads)
    : lockMgr_(std::make_unique<LockManager>()),
      pqMgr_(std::make_unique<SimplePQManager>()) {

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
