#include <fstream>
#include <unistd.h>

#include "theater.h"

namespace theater {

std::vector<std::string> Theater::movie_list_;
std::map<std::string, uint32_t> Theater::movie_seat_;
std::vector<ThreadInfo> Theater::thread_info_ = {
    {Customer, 50, },
    {BoxOfficeAgent, 2, },
    {TicketTaker, 1, },
    {ConcessionStandWorker, 1, }};

void Theater::Open(const std::string file_path) {
  std::ifstream moive_list_file;
  moive_list_file.open(file_path);
  std::string line;
  std::string move_name;
  int32_t max_seat;

  if (!moive_list_file) {
    throw std::runtime_error("Cannot open movie file!\n");
  }

  while (std::getline(moive_list_file, line)) {
    for (auto it = line.cbegin(); it != line.cend(); ++it) {
      if ('\t' == *it) {
        move_name = line.substr(0, it - line.cbegin());
        line.erase(0, ++it - line.cbegin());
        max_seat = std::stoi(line);
        movie_list_.push_back(move_name);
        movie_seat_.insert({move_name, max_seat});
        break;
      }
    }
  }

  moive_list_file.close();
}

void Theater::Simulate(void) {
  for (auto &thread : thread_info_) {
    utils::ThreadUtil::CreateThread(thread.start_routine,
                                    thread.thread_num,
                                    thread.threads);
  }

  for (auto &thread : thread_info_) {
    utils::ThreadUtil::JoinThread(thread.threads);
  }
}

void* Theater::Customer(void* argv) {
  uint32_t* thread_id = static_cast<uint32_t*>(argv);
  std::cout << "[Customer][" << *thread_id << "]" << std::endl;
  sleep(1);
  return argv;
}

void* Theater::BoxOfficeAgent(void* argv) {
  uint32_t* thread_id = static_cast<uint32_t*>(argv);
  std::cout << "[BoxOfficeAgent][" << *thread_id << "]" << std::endl;
  sleep(1);
  return argv;
}

void* Theater::TicketTaker(void* argv) {
  uint32_t* thread_id = static_cast<uint32_t*>(argv);
  std::cout << "[TicketTaker][" << *thread_id << "]" << std::endl;
  sleep(1);
  return argv;
}

void* Theater::ConcessionStandWorker(void* argv) {
  uint32_t* thread_id = static_cast<uint32_t*>(argv);
  std::cout << "[ConcessionStandWorker][" << *thread_id << "]" << std::endl;
  sleep(1);
  return argv;
}

} // namespace theater
