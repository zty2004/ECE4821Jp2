//
// Threadpool
// When the threadpool has empty slot, it asks scheduler for tasks. This class
// focues on thread management and synchronization.
//

#include "Threadpool.h"
#include "../query/QueryHelpers.h"

template <std::size_t PoolSize>
Threadpool<PoolSize>::Threadpool(LockManager &lm, TaskQueue &tq)
    : lock_manager_(lm), task_queue_(tq) {
  for (std::size_t i = 0; i < PoolSize; ++i) {
#ifdef __cpp_lib_jthread
    threads[i] =
        thread_t([this](std::stop_token st) { this->worker_loop(st); });
#else
    threads[i] = thread_t([this] { this->worker_loop(); });
#endif
  }
}

template <std::size_t PoolSize> Threadpool<PoolSize>::~Threadpool() {
  stop_flag_.store(true);
#ifndef __cpp_lib_jthread
  for (auto &t : threads) {
    if (t.joinable()) {
      t.join();
    }
  }
#endif
}

template <std::size_t PoolSize>
auto Threadpool<PoolSize>::get_threadpool_size() const -> size_t {
  return thread_count;
}

template <std::size_t PoolSize>
Threadpool<PoolSize>::WriteGuard::WriteGuard(LockManager &lkm, TableId index)
    : lm_(lkm), id_(std::move(index)) {
  lm_.lockX(id_);
}

template <std::size_t PoolSize>
Threadpool<PoolSize>::WriteGuard::~WriteGuard() {
  lm_.unlockX(id_);
}

template <std::size_t PoolSize>
Threadpool<PoolSize>::ReadGuard::ReadGuard(LockManager &lkm, TableId index)
    : lm_(lkm), id_(std::move(index)) {
  lm_.lockS(id_);
}

template <std::size_t PoolSize> Threadpool<PoolSize>::ReadGuard::~ReadGuard() {
  lm_.unlockS(id_);
}

#ifdef __cpp_lib_jthread
template <std::size_t PoolSize>
void Threadpool<PoolSize>::worker_loop(const std::stop_token &st) {
  while (!st.stop_requested()) {
    work();
  }
}
#else
template <std::size_t PoolSize> void Threadpool<PoolSize>::worker_loop() {
  while (!stop_flag_.load(std::memory_order_acquire)) {
    work();
  }
}
#endif

template <std::size_t PoolSize> void Threadpool<PoolSize>::work() {
  ExecutableTask task;
  bool has_task = false;
  {
    std::unique_lock<std::mutex> lock(local_mutex_);
    if (local_queue_.empty()) {
      lock.unlock();

      std::vector<ExecutableTask> batch;
      batch.reserve(FETCH_BATCH_SIZE);

      for (size_t i = 0; i < FETCH_BATCH_SIZE; ++i) {
        ExecutableTask tmp;
        if (task_queue_.fetchNext(tmp)) {
          batch.push_back(std::move(tmp));
        } else {
          break;
        }
      }

      lock.lock();
      for (auto &tmp : batch) {
        local_queue_.emplace(std::move(tmp));
      }
    }

    if (!local_queue_.empty()) {
      task = std::move(local_queue_.front());
      local_queue_.pop();
      has_task = true;
    }
  }

  if (has_task) {
    executeTask(task);
  } else {
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
  }
}

template <std::size_t PoolSize>
void Threadpool<PoolSize>::executeTask(ExecutableTask &task) {
  if (!task.query) {
    executeNull(task);
    return;
  }

  const TableId tid = resolveTableId(*task.query);
  const QueryKind kind = getQueryKind(queryType(*task.query));

  try {
    if (kind == QueryKind::Write) {
      WriteGuard guard(lock_manager_, tid);
      executeWrite(task);
    } else if (kind == QueryKind::Read) {
      ReadGuard guard(lock_manager_, tid);
      executeRead(task);
    } else {
      executeNull(task);
    }
  } catch (...) {
  }
}

template <std::size_t PoolSize>
void Threadpool<PoolSize>::executeWrite(ExecutableTask &task) {
  run_logic(task, "WRITE");
}

template <std::size_t PoolSize>
void Threadpool<PoolSize>::executeRead(ExecutableTask &task) {
  run_logic(task, "READ");
}

template <std::size_t PoolSize>
void Threadpool<PoolSize>::executeNull(ExecutableTask &task) {
  run_logic(task, "NULL");
}

template <std::size_t PoolSize>
void Threadpool<PoolSize>::run_logic(ExecutableTask &task, const char *type) {
  try {
    std::unique_ptr<QueryResult> res;
    if (task.execOverride) {
      res = task.execOverride();
    } else {
      res = std::make_unique<QueryResult>(std::string(type) + " Result");
    }
    task.promise.set_value(std::move(res));
  } catch (...) {
    try {
      task.promise.set_exception(std::current_exception());
    } catch (...) {
    }
  }
  if (task.onCompleted) {
    task.onCompleted();
  }
}