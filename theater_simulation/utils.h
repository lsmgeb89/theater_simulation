#ifndef THEATER_SIMULATION_UTILS_H_
#define THEATER_SIMULATION_UTILS_H_

#include <pthread.h>
#include <iostream>
#include <stdexcept>

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
        delete status;
      }
    }
  };
};

} // namespace utils

#endif // THEATER_SIMULATION_UTILS_H_
