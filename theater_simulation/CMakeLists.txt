cmake_minimum_required(VERSION 3.3)

project(theater_simulation)

file(GLOB SOURCE_FILES "*.h" "*.cc")

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

set(THREADS_PREFER_PTHREAD_FLAG ON)

find_package(Threads REQUIRED)

add_executable(theater_simulation ${SOURCE_FILES})

target_link_libraries(theater_simulation Threads::Threads)