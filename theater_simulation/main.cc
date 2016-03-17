#include "theater.h"

int main(int argc, char* argv[]) {
  if (argc != 2) {
    std::cerr << "Wrong Parameter!" << std::endl;
    return -1;
  }

  try {
    theater::Theater::Init();
    theater::Theater::Open(argv[1]);
    theater::Theater::Simulate();
  } catch (const std::runtime_error& err) {
    std::cerr << err.what();
  }
  return 0;
}