#include "theater.h"

int main(int argc, char* argv[]) {
  if (argc != 2) {
    std::cerr << "Wrong Parameter!" << std::endl;
    return -1;
  }

  try {
    theater::Theater t;
    t.Open(argv[1]);
    t.Simulate();
  } catch (const std::runtime_error& err) {
    std::cerr << err.what();
  }
  return 0;
}
