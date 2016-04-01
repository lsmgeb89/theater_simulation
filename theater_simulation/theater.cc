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
    mutex_buy_ticket_queue_(1),
    message_(customer_num),
    buy_ticket_finished_(customer_num),
    mutex_ticket_taker_queue_(1),
    ticket_taken_(customer_num),
    mutex_concession_stand_queue_(1),
    food_message_(customer_num),
    buy_food_finished_(customer_num) {
  thread_pool_.push_back({&Theater::BoxOfficeAgent, agent_num, "Box Office Agent"});
  thread_pool_.push_back({&Theater::TicketTaker, taker_num, "Ticket Taker"});
  thread_pool_.push_back({&Theater::ConcessionStandWorker, worker_num, "Concession Stand Worker"});
  thread_pool_.push_back({&Theater::Customer, customer_num, "Customer"});

  random_engine_.seed(std::random_device()());
}

bool Theater::BuyTicket(const uint32_t &id) {
  bool result = true;

  {
    utils::SemaphoreGuard lock(mutex_buy_ticket_queue_);
    buy_ticket_queue_.push(id);

    message_[id].movie_index = (dist_(random_engine_) % movie_list_.size());

    MUTEX_COUT << "Customer " << id << " created, buying ticket to "
               << movie_list_[message_[id].movie_index] << std::endl;

    // Notify an agent someone has in line
    customer_in_buy_ticket_queue_.Post();
  }

  // Wait the agent to check whether there is a ticket
  buy_ticket_finished_[id].Wait();

  if (TicketSold == message_[id].result) {
    MUTEX_COUT << "Customer " << id << " goes home because all tickets of "
               << movie_list_[message_[id].movie_index] << " have been sold out" << std::endl;
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

    food_message_[id] = static_cast<Food>(dist_(random_engine_) % 3);
    MUTEX_COUT << "Customer " << id << " in line to buy "
               << FoodToString(food_message_[id]) << std::endl;
    customer_in_concession_stand_queue_.Post();
  }

  // Wait the concession stand worker to give me the food
  buy_food_finished_[id].Wait();

  MUTEX_COUT << "Customer " << id << " receives "
             << FoodToString(food_message_[id]) << std::endl;
}

void Theater::Customer(const uint32_t& id, std::promise<uint32_t> id_promise) {

  if (!BuyTicket(id)) { goto done; }

  TakeTicket(id);

  if (dist_(random_engine_) % 2) { BuyFood(id); }

  MUTEX_COUT << "Customer " << id << " enters theater to see "
             << movie_list_[message_[id].movie_index] << std::endl;

done:
  id_promise.set_value(id);
}

void Theater::BoxOfficeAgent(const uint32_t& id, std::promise<uint32_t> id_promise) {
  uint32_t customer_id;

  MUTEX_COUT << "Box office agent " << id << " created" << std::endl;

  while(!done_) {
    // Wait customers going to line
    customer_in_buy_ticket_queue_.Wait();

    if (done_) { goto done; }

    {
      utils::SemaphoreGuard lock(mutex_buy_ticket_queue_);
      customer_id = buy_ticket_queue_.front();
      buy_ticket_queue_.pop();
    }

    MUTEX_COUT << "Box office agent " << id << " serving customer " << customer_id << std::endl;

    { // Check whether seat is available
      utils::SemaphoreGuard lock(mutex_movie_seat_);
      if (movie_seat_[message_[customer_id].movie_index] >= 1) {
        movie_seat_[message_[customer_id].movie_index]--;
        message_[customer_id].result = TicketAvailable;

        MUTEX_COUT << "Box office agent " << id << " sold ticket for "
                   << movie_list_[message_[customer_id].movie_index]
                   << " to customer " << customer_id << std::endl;
      } else {
        message_[customer_id].result = TicketSold;
      }
      usleep(1500000);
    }

    // Notify the customer to check the response
    buy_ticket_finished_[customer_id].Post();
  }

done:
  id_promise.set_value(id);
}

void Theater::TicketTaker(const uint32_t& id, std::promise<uint32_t> id_promise) {
  uint32_t customer_id;

  MUTEX_COUT << "Ticket taker " << id << " created" << std::endl;

  while(!done_) {
    // Wait customers going to line
    customer_in_ticket_taker_queue_.Wait();

    if (done_) { goto done; }

    {
      utils::SemaphoreGuard lock(mutex_ticket_taker_queue_);
      customer_id = ticket_taker_queue_.front();
      ticket_taker_queue_.pop();
    }

    usleep(250000);

    MUTEX_COUT << "Ticket taken from customer " << customer_id << std::endl;

    ticket_taken_[customer_id].Post();
  }

done:
  id_promise.set_value(id);
}

void Theater::ConcessionStandWorker(const uint32_t& id, std::promise<uint32_t> id_promise) {
  uint32_t customer_id;

  MUTEX_COUT << "Concession stand worker " << id << " created" << std::endl;

  while(!done_) {
    // Wait customers going to line
    customer_in_concession_stand_queue_.Wait();

    if (done_) { goto done; }

    {
      utils::SemaphoreGuard lock(mutex_concession_stand_queue_);
      customer_id = concession_stand_queue_.front();
      concession_stand_queue_.pop();
    }

    MUTEX_COUT << "Order for " << FoodToString(food_message_[customer_id])
               << " taken from customer " << customer_id << std::endl;

    // Fill Order
    usleep(3000000);

    // Notify the customer to take the food
    MUTEX_COUT << FoodToString(food_message_[customer_id])
               << " given to customer " << customer_id << std::endl;

    buy_food_finished_[customer_id].Post();
  }

done:
  id_promise.set_value(id);
}

void Theater::Simulate(void) {
  for (uint32_t i = 0; i < 3; ++i) {
    CreateThread(thread_pool_[i]);
  }

  MUTEX_COUT << "Theater is open" << std::endl;

  CreateThread(thread_pool_[3]);

  JoinThread(thread_pool_[3]);

  done_ = true;

  for (uint32_t i = 0; i < thread_pool_[0].thread_num; ++i) {
    customer_in_buy_ticket_queue_.Post();
  }

  for (uint32_t i = 0; i < thread_pool_[1].thread_num; ++i) {
    customer_in_ticket_taker_queue_.Post();
  }

  for (uint32_t i = 0; i < thread_pool_[2].thread_num; ++i) {
    customer_in_concession_stand_queue_.Post();
  }

  for (uint32_t i = 0; i < 3; ++i) {
    JoinThread(thread_pool_[i]);
  }
}

void Theater::CreateThread(ThreadInfo& thread_info) {
  try {
    for (uint32_t i = 0; i < thread_info.thread_num; ++i) {
      thread_info.promises.emplace_back();
      thread_info.futures.push_back(thread_info.promises[i].get_future());
      thread_info.threads.push_back(std::thread(thread_info.start_routine,
                                                this,
                                                i,
                                                std::move(thread_info.promises[i])));
    }
  } catch(...) {
    throw;
  }
}

void Theater::JoinThread(ThreadInfo& thread_info) {
  for (uint32_t i = 0; i < thread_info.thread_num; ++i) {
    if (thread_info.threads[i].joinable()) {
      thread_info.threads[i].join();
      MUTEX_COUT << "Joined " << thread_info.name << " "
                 << thread_info.futures[i].get() << std::endl;
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
