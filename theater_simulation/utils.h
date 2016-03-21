#ifndef THEATER_SIMULATION_UTILS_H_
#define THEATER_SIMULATION_UTILS_H_

#include <condition_variable>
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

class SemaphoreGuard {
 public:
  explicit SemaphoreGuard(Semaphore& semaphore)
    : semaphore_(semaphore) {
      semaphore_.Wait();
  }

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

} // namespace utils

#endif //THEATER_SIMULATION_UTILS_H_
