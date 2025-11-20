//
// Threadpool
// Accept one task from Manager, distribute it to a thread. This class focues on
// thread management and synchronization.
//

#ifndef SRC_RUNTIME_THREADPOOL_H_
#define SRC_RUNTIME_THREADPOOL_H_

#define __cpp_lib_jthread

#include <cstddef>

#ifdef __cpp_lib_jthread
/**
 * @brief The type of threads to use. In C++20 and later we use `std::jthread`.
 */
#include <stop_token>
#include <thread>
using thread_t = std::jthread;
#else
/**
 * @brief The type of threads to use. In C++17 we use`std::thread`.
 */
#include <thread>
using thread_t = std::thread;
#endif

template <std::size_t PoolSize> class Threadpool {
private:
  static constexpr std::size_t thread_count = PoolSize;
  std::array<thread_t, PoolSize> threads;

public:
  Threadpool() {
    for (std::size_t i = 0; i < PoolSize; ++i) {
#ifdef __cpp_lib_jthread
      threads[i] = thread_t([this](std::stop_token st) { this->worker(st); });
#else
      threads[i] = thread_t([this] { this->worker(); });
#endif
    }
  }
  Threadpool(const Threadpool &) = delete;
  Threadpool(Threadpool &&) = delete;
  auto operator=(const Threadpool &) -> Threadpool & = delete;
  auto operator=(Threadpool &&) -> Threadpool & = delete;
  [[nodiscard]] auto get_thread_count() const -> size_t {
    return this->threads.size();
  }
#ifndef __cpp_lib_jthread
  /**
   * @brief Manually join all the threads. In C++17, we use std::thread which
   * need manual joining.
   */
  void destroy_threads() {
    {
      const std::scoped_lock tasks_lock(tasks_mutex);
      workers_running = false;
    }
    task_available_cv.notify_all();

    // join all the threads
    for (auto &t : threads) {
      if (t.joinable()) {
        t.join();
      }
    }

    threads.clear();
  }
#endif
};

#endif  // SRC_RUNTIME_THREADPOOL_H_