//
// PopUpExecutor implementation
//

#include "PopUpExecutor.h"
#include "../query/QueryResult.h"
#include <exception>

bool PopUpExecutor::tryPop(QueryTask &out) {
  std::lock_guard lk(qMtx_);
  if (ready_.empty())
    return false;
  out = std::move(ready_.front());
  ready_.pop_front();
  return true;
}

void PopUpExecutor::submit(QueryTask t) {
  {
    std::lock_guard lk(qMtx_);
    ready_.push_back(std::move(t));
  }
  qCv_.notify_one();
  maybeGrow();
}

void PopUpExecutor::maybeGrow() {
  auto act = active_.load(std::memory_order_relaxed);
  std::size_t qsz;
  {
    std::lock_guard lk(qMtx_);
    qsz = ready_.size();
  }
  if (qsz > act && act < kMaxThreads) {
    threads_.emplace_back([this]() { workerLoop(); });
  }
}

void PopUpExecutor::workerLoop() {
  active_.fetch_add(1, std::memory_order_relaxed);

  QueryTask t;
  if (tryPop(t)) {
    runOne(std::move(t));
  } else if (auto pick = pqm_.tryPickNext(); pick.ok) {
    runOne(std::move(pick.task));
  }

  active_.fetch_sub(1, std::memory_order_relaxed);
}

void PopUpExecutor::stop() {
  stopping_.store(true, std::memory_order_release);
  qCv_.notify_all();
  for (auto &thr : threads_) {
    if (thr.joinable())
      thr.join();
  }
  threads_.clear();
}
