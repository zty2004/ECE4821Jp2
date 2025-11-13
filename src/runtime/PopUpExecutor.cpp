//
// PopUpExecutor implementation
//

#include <atomic>
#include <cstddef>
#include <exception>
#include <memory>
#include <mutex>
#include <string>
#include <utility>

#include "../query/QueryResult.h"
#include "PopUpExecutor.h"
#include "QueryTask.h"

auto PopUpExecutor::tryPop(QueryTask &out) -> bool {
  const std::lock_guard lock(qMtx_);
  if (ready_.empty()) {
    return false;
  }
  out = std::move(ready_.front());
  ready_.pop_front();
  return true;
}

void PopUpExecutor::submit(QueryTask task) {
  {
    const std::lock_guard lock(qMtx_);
    ready_.push_back(std::move(task));
  }
  qCv_.notify_one();
  maybeGrow();
}

void PopUpExecutor::maybeGrow() {
  auto act = active_.load(std::memory_order_relaxed);
  std::size_t queueSize = 0;
  {
    const std::lock_guard lock(qMtx_);
    queueSize = ready_.size();
  }
  if (queueSize > act && act < kMaxThreads) {
    threads_.emplace_back([this]() { workerLoop(); });
  }
}

void PopUpExecutor::workerLoop() {
  active_.fetch_add(1, std::memory_order_relaxed);

  QueryTask task;
  if (tryPop(task)) {
    runOne(std::move(task));
  } else if (auto pick = pqm_.tryPickNext(); pick.ok) {
    runOne(std::move(pick.task));
  }

  active_.fetch_sub(1, std::memory_order_relaxed);
}

void PopUpExecutor::stop() {
  stopping_.store(true, std::memory_order_release);
  qCv_.notify_all();
  for (auto &thr : threads_) {
    if (thr.joinable()) {
      thr.join();
    }
  }
  threads_.clear();
}

void PopUpExecutor::runOne(QueryTask task) {
  auto table = task.table;

  if (task.kind == OpKind::Write) {
    if (task.attempts == 0) {
      lm_.writerIntentBegin(table);
    }

    if (lm_.tryLockX(table)) {
      executeWrite(task);
      lm_.unlockX(table);
      lm_.writerIntentEnd(table);
      return;
    }

    task.attempts++;
    pqm_.requeueFront(std::move(task));
    return;
  }

  if (!lm_.canAdmitShared(table)) {
    task.attempts++;
    pqm_.requeueBack(std::move(task));
    return;
  }

  if (lm_.tryLockS(table)) {
    executeRead(task);
    lm_.unlockS(table);
    return;
  }

  task.attempts++;
  pqm_.requeueBack(std::move(task));
}

void PopUpExecutor::executeRead(QueryTask &task) {
  try {
    QueryResult::Ptr result = task.query->execute();
    resultCb_(task.orderIndex, std::move(result));
  } catch (const std::exception &e) {
    auto errorResult = std::make_unique<ErrorMsgResult>(
        "READ", task.table, std::string("Exception: ") + e.what());
    resultCb_(task.orderIndex, std::move(errorResult));
  } catch (...) {
    auto errorResult = std::make_unique<ErrorMsgResult>(
        "READ", task.table, "Unknown exception occurred");
    resultCb_(task.orderIndex, std::move(errorResult));
  }
}

void PopUpExecutor::executeWrite(QueryTask &task) {
  try {
    QueryResult::Ptr result = task.query->execute();
    resultCb_(task.orderIndex, std::move(result));
  } catch (const std::exception &e) {
    auto errorResult = std::make_unique<ErrorMsgResult>(
        "WRITE", task.table, std::string("Exception: ") + e.what());
    resultCb_(task.orderIndex, std::move(errorResult));
  } catch (...) {
    auto errorResult = std::make_unique<ErrorMsgResult>(
        "WRITE", task.table, "Unknown exception occurred");
    resultCb_(task.orderIndex, std::move(errorResult));
  }
}
