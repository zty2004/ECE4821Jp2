//
// Threadpool
// Accept one task from Manager, distribute it to a thread. This class focues on
// thread management and synchronization.
//

#ifndef SRC_RUNTIME_THREADPOOL_H_
#define SRC_RUNTIME_THREADPOOL_H_

#include <cstddef>
#include <thread>
#include <vector>

#ifdef __cpp_lib_jthread
/**
 * @brief The type of threads to use. In C++20 and later we use `std::jthread`.
 */
using thread_t = std::jthread;
#else
/**
 * @brief The type of threads to use. In C++17 we use`std::thread`.
 */
using thread_t = std::thread;
#endif

class Threadpool {
private:
  std::vector<thread_t> threads;

public:
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