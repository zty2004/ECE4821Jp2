//
// Runtime
//

#ifndef SRC_RUNTIME_RUNTIME_H_
#define SRC_RUNTIME_RUNTIME_H_

#include <cstddef>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

#include "../query/Query.h"
#include "../query/QueryResult.h"
#include "LockManager.h"
#include "PopUpExecutor.h"
#include "SimplePQManager.h"

class Runtime {
public:
  explicit Runtime(std::size_t maxThreads = 8);
  ~Runtime();

  Runtime(const Runtime &) = delete;
  Runtime &operator=(const Runtime &) = delete;

  void submitQuery(Query::Ptr query, std::size_t orderIndex);

  void waitAll();

  auto getResultsInOrder() -> std::vector<QueryResult::Ptr>;

private:
  auto determineOpKind(Query &query) -> OpKind;
  auto extractTableName(const Query &query) -> std::string;
  void resultCallback(std::size_t orderIndex, QueryResult::Ptr result);

  std::unique_ptr<LockManager> lockMgr_;
  std::unique_ptr<SimplePQManager> pqMgr_;
  std::unique_ptr<PopUpExecutor> executor_;

  std::mutex resultMtx_;
  std::map<std::size_t, QueryResult::Ptr> results_;
  std::size_t totalSubmitted_{0};
};

#endif  // SRC_RUNTIME_RUNTIME_H_
