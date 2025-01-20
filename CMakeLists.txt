cmake_minimum_required(VERSION 3.10)
project(Market_Parser)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLGAS "-02 -Wall -Wextra")

include_directories(include)

add_executable(${PROJECT_NAME} src/main.cpp)