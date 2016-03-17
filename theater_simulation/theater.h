#ifndef THEATER_SIMULATION_THEATER_H_
#define THEATER_SIMULATION_THEATER_H_

#include <map>
#include <queue>
#include <vector>
#include <random>

#include "utils.h"

namespace theater {
struct ThreadInfo {
  utils::ThreadFunc start_routine;
  uint32_t thread_num;
  std::vector<pthread_t> threads;
};

enum TicketResult {
  TicketAvailable = 1 << 0,
  TicketSold = 1 << 1
};

enum Food {
  Popcorn = 0,
  Soda = 1,
  Both = 2
};

struct TicketMessage {
  uint64_t movie_index;
  TicketResult result;
};

class Theater {
 public:
  static void Open(const std::string file_path);
  static void Init(void);
  static void Simulate(void);
  static void* Customer(void* argv);
  static void* BoxOfficeAgent(void* argv);
  static void* TicketTaker(void* argv);
  static void* ConcessionStandWorker(void* argv);
  static std::string FoodToString(const Food& food) {
    if (Popcorn == food) {
      return "Popcorn";
    } else if (Soda == food) {
      return "Soda";
    } else if (Both == food) {
      return "Popcorn and Soda";
    } else {
      return "";
    }
  }
 private:
  static sem_t mutex_output_;

  static std::vector<std::string> movie_list_;
  static std::vector<ThreadInfo> thread_info_;

  //
  static std::vector<uint64_t> movie_seat_;
  static sem_t mutex_movie_seat_;

  //
  static std::queue<uint32_t> customer_queue_;
  static sem_t mutex_customer_line_;
  static sem_t customer_in_line_;

  // event for buying ticket
  static std::vector<TicketMessage> message_;
  static std::vector<sem_t> agent_available_;
  static std::vector<sem_t> request_available_;
  static std::vector<sem_t> buy_finished_;

  //
  static std::queue<uint32_t> ticket_taker_queue_;
  static sem_t mutex_ticket_taker_queue_;
  static sem_t customer_in_ticket_taker_queue_;

  // event for taking ticket
  static std::vector<sem_t> ticket_taken_;

  //
  static std::queue<uint32_t> concession_stand_queue_;
  static sem_t mutex_concession_stand_queue_;
  static sem_t customer_in_concession_stand_queue_;

  // event for buying ticket
  static std::vector<Food> food_message_;
  static std::vector<sem_t> concession_stand_woker_available_;
  static std::vector<sem_t> food_request_available_;
  static std::vector<sem_t> buy_food_finished_;

  static std::mt19937 random_engine_;
  static std::uniform_int_distribution<std::mt19937::result_type> dist_;
};

} // namespace theater

#endif // THEATER_SIMULATION_THEATER_H_
