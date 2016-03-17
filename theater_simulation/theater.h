#ifndef THEATER_SIMULATION_THEATER_H_
#define THEATER_SIMULATION_THEATER_H_

#include <map>
#include <vector>

#include "utils.h"

namespace theater {
struct ThreadInfo {
  utils::ThreadFunc start_routine;
  uint32_t thread_num;
  std::vector<pthread_t> threads;
};

class Theater {
 public:
  static void Open(const std::string file_path);
  static void Simulate(void);
  static void* Customer(void* argv);
  static void* BoxOfficeAgent(void* argv);
  static void* TicketTaker(void* argv);
  static void* ConcessionStandWorker(void* argv);
 public:
  static std::vector<std::string> movie_list_;
  static std::map<std::string, uint32_t> movie_seat_;
  static std::vector<ThreadInfo> thread_info_;
};

} // namespace theater

#endif // THEATER_SIMULATION_THEATER_H_
