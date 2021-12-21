message("\n==== simple_paxos ====")
cmake_minimum_required(VERSION 3.5)

set(CMAKE_CXX_STANDARD 17)

project(simple_paxos)

add_executable(thread_paxos thread_paxos.cpp)
target_link_libraries(thread_paxos pthread)
