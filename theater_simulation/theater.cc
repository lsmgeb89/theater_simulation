#include <fstream>
#include <unistd.h>

#include "theater.h"

namespace theater {

void Theater::Open(const std::string file_path) {
  std::ifstream movie_list_file;
  movie_list_file.open(file_path);
  std::string line;
  std::string move_name;
  uint64_t max_seat;

  if (!movie_list_file) {
    throw std::runtime_error("Cannot open movie file!\n");
  }

  while (std::getline(movie_list_file, line)) {
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

  movie_list_file.close();
}

Theater::Theater(const uint32_t agent_num,
                 const uint32_t taker_num,
                 const uint32_t worker_num,
                 const uint32_t customer_num)
  : done_(false),
    mutex_output_(1),
    mutex_movie_seat_(1),
    mutex_customer_line_(1),
    message_(customer_num),
    agent_available_(customer_num),
    request_available_(customer_num),
    buy_finished_(customer_num),
    mutex_ticket_taker_queue_(1),
    ticket_taken_(customer_num),
    mutex_concession_stand_queue_(1),
    food_message_(customer_num),
    concession_stand_worker_available_(customer_num),
    food_request_available_(customer_num),
    buy_food_finished_(customer_num) {
  pools_info_[0] = {&Theater::BoxOfficeAgent, agent_num};
  pools_info_[1] = {&Theater::TicketTaker, taker_num};
  pools_info_[2] = {&Theater::ConcessionStandWorker, worker_num};
  pools_info_[3] = {&Theater::Customer, customer_num};

  random_engine_.seed(std::random_device()());
}

bool Theater::BuyTicket(const uint32_t &id) {
  bool result = true;
  // Get a position in the line
  {
    utils::SemaphoreGuard lock(mutex_customer_line_);
    customer_queue_.push(id);

    MUTEX_COUT << "Customer " << id << " in line" << std::endl;

    // Notify an agent someone has in line
    customer_in_line_.Post();
  }

  // Wait the agent to call me
  agent_available_[id].Wait();

  message_[id].movie_index = (dist_(random_engine_) % movie_list_.size());

  MUTEX_COUT << "Customer " << id << " buying ticket to " << movie_list_[message_[id].movie_index]
        << std::endl;

  // Notify the movie I want to see
  request_available_[id].Post();

  // Wait the agent to check whether there is a ticket
  buy_finished_[id].Wait();

  if (TicketSold == message_[id].result) {
    MUTEX_COUT << "Customer " << id << " return because ticket has sold " << std::endl;
    result = false;
  }
  return result;
}

void Theater::TakeTicket(const uint32_t &id) {
  {
    utils::SemaphoreGuard lock(mutex_ticket_taker_queue_);
    ticket_taker_queue_.push(id);

    MUTEX_COUT << "Customer " << id << " in line to see ticket taker" << std::endl;

    // Notify the ticket taker someone has in line
    customer_in_ticket_taker_queue_.Post();
  }

  ticket_taken_[id].Wait();
}

void Theater::BuyFood(const uint32_t& id) {
  {
    utils::SemaphoreGuard lock(mutex_concession_stand_queue_);
    concession_stand_queue_.push(id);

    MUTEX_COUT << "Customer " << id << " in line to see concession stand" << std::endl;
  }

  customer_in_concession_stand_queue_.Post();

  // Wait the concession stand worker to call me
  concession_stand_worker_available_[id].Wait();

  food_message_[id] = static_cast<Food>(dist_(random_engine_) % 3);

  MUTEX_COUT << "Customer " << id << " in line to buy " << FoodToString(food_message_[id])
        << std::endl;

  // Notify the food I want to eat
  food_request_available_[id].Post();

  // Wait the concession stand worker to give me the food
  buy_food_finished_[id].Wait();

  MUTEX_COUT << "Customer " << id << " receives " << FoodToString(food_message_[id]) << std::endl;
  MUTEX_COUT << "Customer " << id << " enters theater to see "
        << movie_list_[message_[id].movie_index] << std::endl;
}

void Theater::Customer(const uint32_t& id) {
  MUTEX_COUT << "Customer " << id << " created" << std::endl;

  if (!BuyTicket(id)) { return; }

  TakeTicket(id);

  bool to_buy_concession = static_cast<bool>(dist_(random_engine_) % 2);

  if (!to_buy_concession) {
    MUTEX_COUT << "Customer " << id << " enters theater to see "
          << movie_list_[message_[id].movie_index] << std::endl;
    return;
  }

  BuyFood(id);
}

void Theater::BoxOfficeAgent(const uint32_t& id) {
  uint32_t customer_id;

  MUTEX_COUT << "Box office agent " << id << " created" << std::endl;

  while(!done_) {
    // Wait customers going to line
    customer_in_line_.Wait();

    if (done_) { return; }

    MUTEX_COUT << "Box office agent " << id << " sees customers in line" << std::endl;

    {
      utils::SemaphoreGuard lock(mutex_customer_line_);
      customer_id = customer_queue_.front();
      customer_queue_.pop();
    }

    // Notify the front customer that I am ready to serve
    agent_available_[customer_id].Post();

    MUTEX_COUT << "Box office agent " << id << " serving customer " << customer_id << std::endl;

    // Wait the movie the customer want to see
    request_available_[customer_id].Wait();

    // Check whether seat is available
    {
      utils::SemaphoreGuard lock(mutex_movie_seat_);
      if (movie_seat_[message_[customer_id].movie_index] >= 1) {
        movie_seat_[message_[customer_id].movie_index]--;
        message_[customer_id].result = TicketAvailable;

        MUTEX_COUT << "Box office agent " << id << " sold ticket for "
              << movie_list_[message_[customer_id].movie_index];
        MUTEX_COUT << " to customer " << customer_id << std::endl;

        usleep(1500);
      }
    }

    // Notify the customer to check the response
    buy_finished_[customer_id].Post();
  }
}

void Theater::TicketTaker(const uint32_t& id) {
  uint32_t customer_id;

  MUTEX_COUT << "Ticket taker " << id << " created" << std::endl;

  while(!done_) {
    // Wait customers going to line
    customer_in_ticket_taker_queue_.Wait();

    if (done_) { return; }

    MUTEX_COUT << "Ticket taker " << id << " sees customers in line" << std::endl;

    {
      utils::SemaphoreGuard lock(mutex_ticket_taker_queue_);
      customer_id = ticket_taker_queue_.front();
      ticket_taker_queue_.pop();
    }

    usleep(250);

    MUTEX_COUT << "Ticket taken from customer " << customer_id << std::endl;

    ticket_taken_[customer_id].Post();
  }
}

void Theater::ConcessionStandWorker(const uint32_t& id) {
  uint32_t customer_id;

  MUTEX_COUT << "Concession stand worker " << id << " created" << std::endl;

  while(!done_) {
    // Wait customers going to line
    customer_in_concession_stand_queue_.Wait();

    if (done_) { return; }

    MUTEX_COUT << "Concession stand worker " << id << " sees customers in line" << std::endl;

    {
      utils::SemaphoreGuard lock(mutex_concession_stand_queue_);
      customer_id = concession_stand_queue_.front();
      concession_stand_queue_.pop();
    }

    // Notify the front customer that I am ready to serve
    concession_stand_worker_available_[customer_id].Post();

    MUTEX_COUT << "Concession stand worker " << id << " serving customer " << customer_id << std::endl;

    // Wait the food the customer want to eat
    food_request_available_[customer_id].Wait();

    // Check whether seat is available
    MUTEX_COUT << "Order for " << FoodToString(food_message_[customer_id]) << " taken from customer " << customer_id
          << std::endl;

    // Fill Order
    usleep(3000);

    // Notify the customer to take the food
    MUTEX_COUT << FoodToString(food_message_[customer_id]) << " given to customer " << customer_id << std::endl;

    buy_food_finished_[customer_id].Post();
  }
}

void Theater::Simulate(void) {
  for (uint32_t i = 0; i < 3; ++i) {
    CreateThread(pools_info_[i], threads_[i]);
  }

  CreateThread(pools_info_[3], threads_[3]);

  MUTEX_COUT << "Theater is open" << std::endl;

  JoinThread(threads_[3]);

  done_ = true;

  for (uint32_t i = 0; i < pools_info_[0].thread_num; ++i) {
    customer_in_line_.Post();
  }

  for (uint32_t i = 0; i < pools_info_[1].thread_num; ++i) {
    customer_in_ticket_taker_queue_.Post();
  }

  for (uint32_t i = 0; i < pools_info_[2].thread_num; ++i) {
    customer_in_concession_stand_queue_.Post();
  }

  for (uint32_t i = 0; i < 3; ++i) {
    JoinThread(threads_[i]);
  }
}

void Theater::CreateThread(const ThreadInfo& thread_info,
                           std::vector<std::thread>& threads) {
  try {
    for (uint32_t i = 0; i < thread_info.thread_num; ++i) {
      threads.push_back(std::thread(thread_info.start_routine, this, i));
    }
  } catch(...) {
    throw;
  }
}

void Theater::JoinThread(std::vector<std::thread>& threads) {
  for (auto& thread : threads) {
    if (thread.joinable()) {
      std::thread::id thread_id = thread.get_id();
      thread.join();
      MUTEX_COUT << "Join thread " << thread_id << std::endl;
    }
  }
}

std::string Theater::FoodToString(const Food& food) {
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

} // namespace theater
