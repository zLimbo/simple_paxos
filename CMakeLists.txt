message("==== simple_paxos ====")
cmake_minimum_required(VERSION 3.5)

set(CMAKE_CXX_STANDARD 17)

project(simple_paxos)

add_executable(simple_paxos simple_paxos.cc)
target_link_libraries(simple_paxos pthread)
