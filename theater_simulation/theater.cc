#include <fstream>
#include <unistd.h>

#include "theater.h"

namespace theater {
// static members
sem_t Theater::mutex_output_;

std::vector<std::string> Theater::movie_list_;
std::vector<uint64_t> Theater::movie_seat_;
std::vector<ThreadInfo> Theater::thread_info_;
std::queue<uint32_t> Theater::customer_queue_;
sem_t Theater::mutex_customer_line_;
sem_t Theater::mutex_movie_seat_;
sem_t Theater::customer_in_line_;
std::vector<TicketMessage> Theater::message_;
std::vector<sem_t> Theater::buy_finished_;
std::vector<sem_t> Theater::request_available_;
std::vector<sem_t> Theater::agent_available_;

std::queue<uint32_t> Theater::ticket_taker_queue_;
sem_t Theater::mutex_ticket_taker_queue_;
sem_t Theater::customer_in_ticket_taker_queue_;

std::queue<uint32_t> Theater::concession_stand_queue_;
sem_t Theater::mutex_concession_stand_queue_;
sem_t Theater::customer_in_concession_stand_queue_;

std::vector<sem_t> Theater::ticket_taken_;

std::vector<Food> Theater::food_message_;
std::vector<sem_t> Theater::concession_stand_woker_available_;
std::vector<sem_t> Theater::food_request_available_;
std::vector<sem_t> Theater::buy_food_finished_;

std::uniform_int_distribution<std::mt19937::result_type> Theater::dist_(1, 100);
std::mt19937 Theater::random_engine_;

void Theater::Open(const std::string file_path) {
  std::ifstream moive_list_file;
  moive_list_file.open(file_path);
  std::string line;
  std::string move_name;
  uint64_t max_seat;

  if (!moive_list_file) {
    throw std::runtime_error("Cannot open movie file!\n");
  }

  while (std::getline(moive_list_file, line)) {
    for (auto it = line.cbegin(); it != line.cend(); ++it) {
      if ('\t' == *it) {
        move_name = line.substr(0, it - line.cbegin());
        line.erase(0, ++it - line.cbegin());
        max_seat = std::stoul(line);
        movie_list_.push_back(move_name);
        movie_seat_.push_back(max_seat);
        break;
      }
    }
  }

  moive_list_file.close();
}

void Theater::Init(void) {
  sem_t tmp;
  thread_info_ = {{BoxOfficeAgent, 2, }, {TicketTaker, 1, }, {ConcessionStandWorker, 1, }, {Customer, 50, }};

  random_engine_.seed(std::random_device()());

  utils::SemUtil::Init(&mutex_output_, 1);

  utils::SemUtil::Init(&mutex_customer_line_, 1);
  utils::SemUtil::Init(&mutex_movie_seat_, 1);
  utils::SemUtil::Init(&customer_in_line_, 0);

  utils::SemUtil::Init(&mutex_ticket_taker_queue_, 1);
  utils::SemUtil::Init(&customer_in_ticket_taker_queue_, 0);

  utils::SemUtil::Init(&mutex_concession_stand_queue_, 1);
  utils::SemUtil::Init(&customer_in_concession_stand_queue_, 0);

  for (uint32_t i = 0; i < thread_info_.back().thread_num; ++i) {
    message_.push_back({0, TicketSold});
    food_message_.push_back(Popcorn);

    utils::SemUtil::Init(&tmp, 0);

    agent_available_.push_back(tmp);
    request_available_.push_back(tmp);
    buy_finished_.push_back(tmp);

    ticket_taken_.push_back(tmp);

    concession_stand_woker_available_.push_back(tmp);
    food_request_available_.push_back(tmp);
    buy_food_finished_.push_back(tmp);
  }
}

void Theater::Simulate(void) {
  for (auto &thread : thread_info_) {
    utils::ThreadUtil::CreateThread(thread.start_routine,
                                    thread.thread_num,
                                    thread.threads);
  }

  utils::ThreadUtil::JoinThread(thread_info_.back().threads);
}

void* Theater::Customer(void* argv) {
  uint32_t* customer_id = static_cast<uint32_t*>(argv);
  bool to_buy_concession;

  utils::SemUtil::Wait(&mutex_output_);
  std::cout << "Customer " << *customer_id << " created" << std::endl;
  utils::SemUtil::Post(&mutex_output_);

  // Get a position in the line
  utils::SemUtil::Wait(&mutex_customer_line_);
  customer_queue_.push(*customer_id);

  utils::SemUtil::Wait(&mutex_output_);
  std::cout << "Customer " << *customer_id << " in line" << std::endl;
  utils::SemUtil::Post(&mutex_output_);

  // Notify an agent someone has in line
  utils::SemUtil::Post(&customer_in_line_);
  utils::SemUtil::Post(&mutex_customer_line_);

  // Wait the agent to call me
  utils::SemUtil::Wait(&agent_available_[*customer_id]);

  message_[*customer_id].movie_index = (dist_(random_engine_) % movie_list_.size());

  utils::SemUtil::Wait(&mutex_output_);
  std::cout << "Customer " << *customer_id << " buying ticket to " << movie_list_[message_[*customer_id].movie_index] << std::endl;
  utils::SemUtil::Post(&mutex_output_);

  // Notify the movie I want to see
  utils::SemUtil::Post(&request_available_[*customer_id]);

  // Wait the agent to check whether there is a ticket
  utils::SemUtil::Wait(&buy_finished_[*customer_id]);

  if (TicketSold == message_[*customer_id].result) {
    utils::SemUtil::Wait(&mutex_output_);
    std::cout << "Customer " << *customer_id << " return because ticket has sold " << std::endl;
    utils::SemUtil::Post(&mutex_output_);
    return argv;
  }

  //----------------------------------------
  // Get a position in the ticket taker line
  utils::SemUtil::Wait(&mutex_ticket_taker_queue_);
  ticket_taker_queue_.push(*customer_id);

  utils::SemUtil::Wait(&mutex_output_);
  std::cout << "Customer " << *customer_id << " in line to see ticket taker" << std::endl;
  utils::SemUtil::Post(&mutex_output_);

  // Notify the ticket taker someone has in line
  utils::SemUtil::Post(&customer_in_ticket_taker_queue_);
  utils::SemUtil::Post(&mutex_ticket_taker_queue_);

  utils::SemUtil::Wait(&ticket_taken_[*customer_id]);

  to_buy_concession = static_cast<bool>(dist_(random_engine_) % 2);

  if (!to_buy_concession) {
    utils::SemUtil::Wait(&mutex_output_);
    std::cout << "Customer " << *customer_id << " enters theater to see " << movie_list_[message_[*customer_id].movie_index] << std::endl;
    utils::SemUtil::Post(&mutex_output_);
    return argv;
  }

  //--------------------------------------
  // Get a position in the concession line
  utils::SemUtil::Wait(&mutex_concession_stand_queue_);
  concession_stand_queue_.push(*customer_id);

  utils::SemUtil::Wait(&mutex_output_);
  std::cout << "Customer " << *customer_id << " in line to see concession stand" << std::endl;
  utils::SemUtil::Post(&mutex_output_);

  // Notify the concession stand someone has in line
  utils::SemUtil::Post(&mutex_concession_stand_queue_);
  utils::SemUtil::Post(&customer_in_concession_stand_queue_);

  // Wait the concession stand worker to call me
  utils::SemUtil::Wait(&concession_stand_woker_available_[*customer_id]);

  food_message_[*customer_id] = static_cast<Food>(dist_(random_engine_) % 3);

  utils::SemUtil::Wait(&mutex_output_);
  std::cout << "Customer " << *customer_id << " in line to buy " << FoodToString(food_message_[*customer_id]) << std::endl;
  utils::SemUtil::Post(&mutex_output_);

  // Notify the food I want to eat
  utils::SemUtil::Post(&food_request_available_[*customer_id]);

  // Wait the concession stand worker to give me the food
  utils::SemUtil::Wait(&buy_food_finished_[*customer_id]);

  utils::SemUtil::Wait(&mutex_output_);
  std::cout << "Customer " << *customer_id << " receives " << FoodToString(food_message_[*customer_id]) << std::endl;
  std::cout << "Customer " << *customer_id << " enters theater to see " << movie_list_[message_[*customer_id].movie_index] << std::endl;
  utils::SemUtil::Post(&mutex_output_);

  return argv;
}

void* Theater::BoxOfficeAgent(void* argv) {
  uint32_t customer_id;
  uint32_t* agent_id = static_cast<uint32_t*>(argv);

  utils::SemUtil::Wait(&mutex_output_);
  std::cout << "Box office agent " << *agent_id << " created" << std::endl;
  utils::SemUtil::Post(&mutex_output_);

  while(agent_id) {
    // Wait customers going to line
    utils::SemUtil::Wait(&customer_in_line_);

    utils::SemUtil::Wait(&mutex_output_);
    std::cout << "Box office agent " << *agent_id << " sees customers in line" << std::endl;
    utils::SemUtil::Post(&mutex_output_);

    utils::SemUtil::Wait(&mutex_customer_line_);
    customer_id = customer_queue_.front();
    customer_queue_.pop();
    utils::SemUtil::Post(&mutex_customer_line_);

    // Notify the front customer that I am ready to serve
    utils::SemUtil::Post(&agent_available_[customer_id]);

    utils::SemUtil::Wait(&mutex_output_);
    std::cout << "Box office agent " << *agent_id << " serving customer " << customer_id << std::endl;
    utils::SemUtil::Post(&mutex_output_);

    // Wait the movie the customer want to see
    utils::SemUtil::Wait(&request_available_[customer_id]);

    // Check whether seat is available
    utils::SemUtil::Wait(&mutex_movie_seat_);
    if (movie_seat_[message_[customer_id].movie_index] >= 1) {
      movie_seat_[message_[customer_id].movie_index]--;
      message_[customer_id].result = TicketAvailable;

      utils::SemUtil::Wait(&mutex_output_);
      std::cout << "Box office agent " << *agent_id << " sold ticket for "
          << movie_list_[message_[customer_id].movie_index];
      std::cout << " to customer " << customer_id << std::endl;
      utils::SemUtil::Post(&mutex_output_);

      usleep(1500);
    }
    utils::SemUtil::Post(&mutex_movie_seat_);

    // Notify the customer to check the response
    utils::SemUtil::Post(&buy_finished_[customer_id]);
  }
}

void* Theater::TicketTaker(void* argv) {
  uint32_t customer_id;
  uint32_t* ticket_taker_id = static_cast<uint32_t*>(argv);

  utils::SemUtil::Wait(&mutex_output_);
  std::cout << "Ticket taker " << *ticket_taker_id << " created" << std::endl;
  utils::SemUtil::Post(&mutex_output_);

  while(ticket_taker_id) {
    // Wait customers going to line
    utils::SemUtil::Wait(&customer_in_ticket_taker_queue_);

    utils::SemUtil::Wait(&mutex_output_);
    std::cout << "Ticket taker " << *ticket_taker_id << " sees customers in line" << std::endl;
    utils::SemUtil::Post(&mutex_output_);

    utils::SemUtil::Wait(&mutex_ticket_taker_queue_);
    customer_id = ticket_taker_queue_.front();
    ticket_taker_queue_.pop();
    utils::SemUtil::Post(&mutex_ticket_taker_queue_);

    usleep(250);

    utils::SemUtil::Wait(&mutex_output_);
    std::cout << "Ticket taken from customer " << customer_id << std::endl;
    utils::SemUtil::Post(&mutex_output_);

    utils::SemUtil::Post(&ticket_taken_[customer_id]);
  }
}

void* Theater::ConcessionStandWorker(void* argv) {
  uint32_t customer_id;
  uint32_t* concession_stand_worker_id = static_cast<uint32_t*>(argv);

  utils::SemUtil::Wait(&mutex_output_);
  std::cout << "Concession stand worker " << *concession_stand_worker_id << " created" << std::endl;
  utils::SemUtil::Post(&mutex_output_);

  while(concession_stand_worker_id) {
    // Wait customers going to line
    utils::SemUtil::Wait(&customer_in_concession_stand_queue_);

    utils::SemUtil::Wait(&mutex_output_);
    std::cout << "Concession stand worker " << *concession_stand_worker_id << " sees customers in line" << std::endl;
    utils::SemUtil::Post(&mutex_output_);

    utils::SemUtil::Wait(&mutex_concession_stand_queue_);
    customer_id = concession_stand_queue_.front();
    concession_stand_queue_.pop();
    utils::SemUtil::Post(&mutex_concession_stand_queue_);

    // Notify the front customer that I am ready to serve
    utils::SemUtil::Post(&concession_stand_woker_available_[customer_id]);

    utils::SemUtil::Wait(&mutex_output_);
    std::cout << "Concession stand worker " << *concession_stand_worker_id << " serving customer " << customer_id << std::endl;
    utils::SemUtil::Post(&mutex_output_);

    // Wait the food the customer want to eat
    utils::SemUtil::Wait(&food_request_available_[customer_id]);

    // Check whether seat is available
    utils::SemUtil::Wait(&mutex_output_);
    std::cout << "Order for " << FoodToString(food_message_[customer_id]) << " taken from customer " << customer_id << std::endl;
    utils::SemUtil::Post(&mutex_output_);

    // Fill Order
    usleep(3000);

    // Notify the customer to take the food
    utils::SemUtil::Wait(&mutex_output_);
    std::cout << FoodToString(food_message_[customer_id]) << " given to customer " << customer_id << std::endl;
    utils::SemUtil::Post(&mutex_output_);

    utils::SemUtil::Post(&buy_food_finished_[customer_id]);
  }
}

} // namespace theater
