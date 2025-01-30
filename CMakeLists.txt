cmake_minimum_required(VERSION 3.10)
project(Market_Parser)

#Define data folder path

set(DATA_FOLDER "${CMAKE_SOURCE_DIR}/data")

add_compile_definitions(DATA_FOLDER="${DATA_FOLDER}")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLGAS "-02 -Wall -Wextra")

include_directories(include)

add_executable(${PROJECT_NAME} src/main.cpp src/DataParser.cpp src/Logger.cpp src/BenchMark.cpp)