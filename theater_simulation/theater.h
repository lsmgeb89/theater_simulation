#ifndef THEATER_SIMULATION_THEATER_H_
#define THEATER_SIMULATION_THEATER_H_

#include <atomic>
#include <functional>
#include <iostream>
#include <queue>
#include <random>
#include <string>
#include <thread>

#include "utils.h"

namespace theater {

class Theater;
typedef void (Theater::*ThreadFunc) (const uint32_t&);

struct ThreadInfo {
  ThreadFunc start_routine;
  uint32_t thread_num;
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
  Theater(const uint32_t agent_num = 2,
          const uint32_t taker_num = 1,
          const uint32_t worker_num = 1,
          const uint32_t customer_num = 50);

  void Open(const std::string file_path);
  void Simulate(void);

 private:
  // customer subroutine
  bool BuyTicket(const uint32_t& id);
  void TakeTicket(const uint32_t& id);
  void BuyFood(const uint32_t& id);

  // thread function
  void Customer(const uint32_t& id);
  void BoxOfficeAgent(const uint32_t& id);
  void TicketTaker(const uint32_t& id);
  void ConcessionStandWorker(const uint32_t& id);

  // internal helper
  void CreateThread(const ThreadInfo& thread_info,
                    std::vector<std::thread>& threads);
  void JoinThread(std::vector<std::thread>& threads);
  static std::string FoodToString(const Food& food);

  //
  std::atomic_bool done_;
  utils::Semaphore mutex_output_;
  std::array<ThreadInfo, 4> pools_info_;
  std::array<std::vector<std::thread>, 4> threads_;

  //
  std::vector<uint64_t> movie_seat_;
  std::vector<std::string> movie_list_;
  utils::Semaphore mutex_movie_seat_;

  //
  std::queue<uint32_t> customer_queue_;
  utils::Semaphore mutex_customer_line_;
  utils::Semaphore customer_in_line_;

  // event for buying ticket
  std::vector<TicketMessage> message_;
  std::vector<utils::Semaphore> agent_available_;
  std::vector<utils::Semaphore> request_available_;
  std::vector<utils::Semaphore> buy_finished_;

  //
  std::queue<uint32_t> ticket_taker_queue_;
  utils::Semaphore mutex_ticket_taker_queue_;
  utils::Semaphore customer_in_ticket_taker_queue_;

  // event for taking ticket
  std::vector<utils::Semaphore> ticket_taken_;

  //
  std::queue<uint32_t> concession_stand_queue_;
  utils::Semaphore mutex_concession_stand_queue_;
  utils::Semaphore customer_in_concession_stand_queue_;

  // event for buying ticket
  std::vector<Food> food_message_;
  std::vector<utils::Semaphore> concession_stand_worker_available_;
  std::vector<utils::Semaphore> food_request_available_;
  std::vector<utils::Semaphore> buy_food_finished_;

  std::mt19937 random_engine_;
  std::uniform_int_distribution<std::mt19937::result_type> dist_;
};

} // namespace theater

#endif // THEATER_SIMULATION_THEATER_H_
