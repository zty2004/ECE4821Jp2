//
// PopUpExecutor implementation
//

#include "PopUpExecutor.h"
#include "../query/QueryResult.h"
#include "QueryTask.h"
#include <atomic>
#include <cstddef>
#include <exception>
#include <memory>
#include <mutex>
#include <string>
#include <utility>

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

void PopUpExecutor::runOne(QueryTask t) {
  auto table = t.table;

  if (t.kind == OpKind::Write) {
    if (t.attempts == 0) {
      lm_.writerIntentBegin(table);
    }

    if (lm_.tryLockX(table)) {
      executeWrite(t);
      lm_.unlockX(table);
      lm_.writerIntentEnd(table);
      return;
    }

    t.attempts++;
    pqm_.requeueFront(std::move(t));
    return;
  }

  if (!lm_.canAdmitShared(table)) {
    t.attempts++;
    pqm_.requeueBack(std::move(t));
    return;
  }

  if (lm_.tryLockS(table)) {
    executeRead(t);
    lm_.unlockS(table);
    return;
  }

  t.attempts++;
  pqm_.requeueBack(std::move(t));
}

void PopUpExecutor::executeRead(QueryTask &t) {
  try {
    QueryResult::Ptr result = t.query->execute();
    resultCb_(t.orderIndex, std::move(result));
  } catch (const std::exception &e) {
    auto errorResult = std::make_unique<ErrorMsgResult>(
        "READ", t.table, std::string("Exception: ") + e.what());
    resultCb_(t.orderIndex, std::move(errorResult));
  } catch (...) {
    auto errorResult = std::make_unique<ErrorMsgResult>(
        "READ", t.table, "Unknown exception occurred");
    resultCb_(t.orderIndex, std::move(errorResult));
  }
}

void PopUpExecutor::executeWrite(QueryTask &t) {
  try {
    QueryResult::Ptr result = t.query->execute();
    resultCb_(t.orderIndex, std::move(result));
  } catch (const std::exception &e) {
    auto errorResult = std::make_unique<ErrorMsgResult>(
        "WRITE", t.table, std::string("Exception: ") + e.what());
    resultCb_(t.orderIndex, std::move(errorResult));
  } catch (...) {
    auto errorResult = std::make_unique<ErrorMsgResult>(
        "WRITE", t.table, "Unknown exception occurred");
    resultCb_(t.orderIndex, std::move(errorResult));
  }
}
