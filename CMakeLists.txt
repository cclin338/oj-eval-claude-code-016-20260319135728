cmake_minimum_required(VERSION 3.10)
project(FileStorageBPT)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O2 -Wall -Wextra")

# Add current directory to include path
include_directories(${CMAKE_CURRENT_SOURCE_DIR})

add_executable(code main.cpp)