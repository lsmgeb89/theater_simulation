#ifndef THEATER_SIMULATION_THEATER_H_
#define THEATER_SIMULATION_THEATER_H_

#include <atomic>
#include <future>
#include <queue>
#include <random>
#include <string>
#include <thread>

#include "utils.h"

namespace theater {

class Theater;
typedef void (Theater::*ThreadFunc) (const uint32_t&, std::promise<uint32_t>);

struct ThreadInfo {
  ThreadFunc start_routine;
  uint32_t thread_num;
  std::string name;
  std::vector<std::thread> threads;
  std::vector<std::promise<uint32_t>> promises;
  std::vector<std::future<uint32_t>> futures;
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
  void Customer(const uint32_t& id, std::promise<uint32_t> id_promise);
  void BoxOfficeAgent(const uint32_t& id, std::promise<uint32_t> id_promise);
  void TicketTaker(const uint32_t& id, std::promise<uint32_t> id_promise);
  void ConcessionStandWorker(const uint32_t& id, std::promise<uint32_t> id_promise);

  // internal helper
  void CreateThread(ThreadInfo& thread_info);
  void JoinThread(ThreadInfo& thread_info);
  static std::string FoodToString(const Food& food);

  // control threads
  std::atomic_bool done_;
  utils::Semaphore mutex_output_;
  std::vector<ThreadInfo> thread_pool_;

  // movie info
  std::vector<uint64_t> movie_seat_;
  std::vector<std::string> movie_list_;
  utils::Semaphore mutex_movie_seat_;

  // buy ticket queue related
  std::queue<uint32_t> buy_ticket_queue_;
  utils::Semaphore mutex_buy_ticket_queue_;
  utils::Semaphore customer_in_buy_ticket_queue_;

  // event for buying ticket
  std::vector<TicketMessage> message_;
  std::vector<utils::Semaphore> buy_ticket_finished_;

  // ticket taker queue related
  std::queue<uint32_t> ticket_taker_queue_;
  utils::Semaphore mutex_ticket_taker_queue_;
  utils::Semaphore customer_in_ticket_taker_queue_;

  // event for taking ticket
  std::vector<utils::Semaphore> ticket_taken_;

  // concession stand queue related
  std::queue<uint32_t> concession_stand_queue_;
  utils::Semaphore mutex_concession_stand_queue_;
  utils::Semaphore customer_in_concession_stand_queue_;

  // event for buying food
  std::vector<Food> food_message_;
  std::vector<utils::Semaphore> buy_food_finished_;

  std::mt19937 random_engine_;
  std::uniform_int_distribution<std::mt19937::result_type> dist_;
};

} // namespace theater

#endif // THEATER_SIMULATION_THEATER_H_
