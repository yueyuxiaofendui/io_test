cmake_minimum_required(VERSION 3.3.2)
add_definitions(-std=c++11)
project(io_test)

INCLUDE_DIRECTORIES(include)
AUX_SOURCE_DIRECTORY(src DIR_SRCS)
set(SRC_LIST ${DIR_SRCS})

add_executable(io_test ${SRC_LIST})
find_package(Threads REQUIRED) 
target_link_libraries(io_test Threads::Threads)

SET(CMAKE_BUILD_TYPE "Debug")
SET(CMAKE_CXX_FLAGS_DEBUG "$ENV{CXXFLAGS} -O0 -Wall -g2 -ggdb")
SET(CMAKE_CXX_FLAGS_RELEASE "$ENV{CXXFLAGS} -O3 -Wall")
