cmake_minimum_required(VERSION 3.10)
project(Market_Parser)

# Define data folder path
set(DATA_FOLDER "${CMAKE_SOURCE_DIR}/data")
add_compile_definitions(DATA_FOLDER="${DATA_FOLDER}")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS "-Wall -Wextra")  # Fixed typo: FLGAS → FLAGS

# Find required packages
find_package(CURL REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(Boost REQUIRED COMPONENTS system thread)

# Include directories
include_directories(${CURL_INCLUDE_DIRS})
include_directories(include)
include_directories(${Boost_INCLUDE_DIRS})

# Add nlohmann/json using FetchContent
include(FetchContent)
FetchContent_Declare(json
  GIT_REPOSITORY https://github.com/nlohmann/json.git
  GIT_TAG v3.11.2)
FetchContent_MakeAvailable(json)

# Main executable
add_executable(${PROJECT_NAME} 
    src/main.cpp
    src/BenchMark.cpp 
    src/DataParser.cpp 
    src/Logger.cpp 
    src/MarketDataServer.cpp 
    src/MarketDataClient.cpp
)

# Link libraries (fixed syntax)
target_link_libraries(${PROJECT_NAME} 
    pthread 
    boost_system
    ${CURL_LIBRARIES}  # Changed from CURL_INCLUDE_DIRS
    OpenSSL::SSL 
    OpenSSL::Crypto
    nlohmann_json::nlohmann_json
)

add_subdirectory(test)