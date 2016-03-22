#ifndef THEATER_SIMULATION_UTILS_H_
#define THEATER_SIMULATION_UTILS_H_

#include <condition_variable>
#include <iostream>
#include <mutex>
#include <vector>

namespace utils {

class Semaphore {
 public:
  Semaphore(int32_t count = 0)
    : count_(count) {}

  void Post(void) {
    std::unique_lock<std::mutex> lock(mutex_);
    ++count_;
    cv_.notify_one();
  }

  void Wait(void) {
    std::unique_lock<std::mutex> lock(mutex_);
    while (!count_) {
      cv_.wait(lock);
    }
    --count_;
  }

  bool TryWait(void) {
    std::unique_lock<std::mutex> lock(mutex_);
    if (count_) {
      --count_;
      return true;
    } else {
      return false;
    }
  }

 private:
  int32_t count_;
  std::mutex mutex_;
  std::condition_variable cv_;
};

struct adopt_lock_t { };
constexpr adopt_lock_t adopt_lock { };

class SemaphoreGuard {
 public:
  explicit SemaphoreGuard(Semaphore& semaphore)
    : semaphore_(semaphore) {
      semaphore_.Wait();
  }

  SemaphoreGuard(Semaphore& semaphore, adopt_lock_t)
    : semaphore_(semaphore) { }

  ~SemaphoreGuard() {
    semaphore_.Post();
  }

  // no copy
  SemaphoreGuard(const SemaphoreGuard&) = delete;

  // no assignment
  SemaphoreGuard& operator=(const SemaphoreGuard&) = delete;

 private:
  Semaphore& semaphore_;
};

inline std::ostream& operator<<(std::ostream& out, const SemaphoreGuard&) {
  return out;
}

inline SemaphoreGuard Lock(Semaphore& mutex) {
  mutex.Wait();
  return { mutex, adopt_lock };
}

#define MUTEX_COUT std::cout << Lock(mutex_output_)

} // namespace utils

#endif //THEATER_SIMULATION_UTILS_H_
