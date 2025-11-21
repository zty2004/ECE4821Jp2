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
