#ifndef THEATER_SIMULATION_UTILS_H_
#define THEATER_SIMULATION_UTILS_H_

#include <iostream>
#include <stdexcept>
#include <pthread.h>
#include <semaphore.h>

namespace utils {

typedef void *(*ThreadFunc) (void *);

class ThreadUtil {
 public:
  static void CreateThread(ThreadFunc start_routine,
                           uint32_t thread_num,
                           std::vector<pthread_t>& threads) {
    int ret;
    pthread_t tmp;

    for (uint32_t i = 0; i < thread_num; ++i) {
      uint32_t* arg = new uint32_t(i);
      ret = pthread_create(&tmp, nullptr, start_routine, static_cast<void*>(arg));
      if (ret) {
        perror("[pthread_create]");
        throw std::runtime_error("Create thread error!\n");
      } else {
        threads.push_back(tmp);
      }
    }
  }

  static void JoinThread(std::vector<pthread_t>& threads) {
    int ret;
    uint32_t* status(nullptr);

    for (uint32_t i = 0; i < threads.size(); ++i) {
      ret = pthread_join(threads[i], reinterpret_cast<void**>(&status));
      if (ret) {
        perror("[pthread_join]");
        throw std::runtime_error("Join thread error!\n");
      }
      if (i != *status) {
        std::cerr << "thread " << i << "terminated abnormally" << std::endl;
        delete status;
        throw std::runtime_error("thread terminated abnormally!\n");
      } else {
        std::cout << "Joined customer " << *status << std::endl;
        delete status;
      }
    }
  }
};

class SemUtil {
 public:
  static void Init(sem_t* sem, const uint32_t& value) {
    int ret = sem_init(sem, 0, value);
    if (ret) {
      throw std::runtime_error("Init semaphore failed!\n");
    }
  }

  static void Wait(sem_t* sem) {
    int ret = sem_wait(sem);
    if (ret) {
      throw std::runtime_error("Wait semaphore failed!\n");
    }
  }

  static void Post(sem_t* sem) {
    int ret = sem_post(sem);
    if (ret) {
      throw std::runtime_error("Post semaphore failed!\n");
    }
  }
};

} // namespace utils

#endif // THEATER_SIMULATION_UTILS_H_
