
# Market Data Parser

## 📌 Overview
The **Market Data Parser** is a C++ application that processes and analyzes incoming market data in real-time. It consists of a **market data server** that streams data over a **TCP connection**, and a **client** that receives and logs the data. The project is designed with **performance and low latency** in mind, utilizing **Boost.Asio** for networking and **multithreading** for efficient processing.


## 📌 Project Structure
Market_Parser/
├── CMakeLists.txt       # Main CMake build script
├── src/                 # Source code
│   ├── MarketDataServer.cpp  # Handles client connections and data streaming
│   ├── MarketDataClient.cpp  # Connects to server and receives data
│   ├── DataParser.cpp        # Parses market data from CSV
│   ├── Logger.cpp            # Singleton Logger class
│   ├── main.cpp              # Entry point
│
├── include/             # Header files
│   ├── MarketDataServer.hpp
│   ├── MarketDataClient.hpp
│   ├── Logger.hpp
│   ├── DataParser.hpp
│
├── test/                # Test executables
│   ├── CMakeLists.txt   # CMake script for tests
│   ├── TestMarketDataServer.cpp
│   ├── TestMarketDataClient.cpp
│
├── data/                # Contains market data CSV files
│   ├── market_data.csv
│
├── build/               # Compiled binaries
│
└── README.md            # Project documentation


## 📌 Dependencies
This project uses the following libraries:

- **Boost.Asio** (`libboost-system`) – For networking (TCP socket communication)
- **Boost.Thread** (`libboost-thread`) – For managing multiple clients
- **pthread** – For multithreading support

Make sure you have **Boost C++ Libraries** installed before compiling.

## 📌 Installation & Compilation
### **1️⃣ Install Dependencies**
If you haven't installed Boost, run:
```sh
sudo apt update
sudo apt install libboost-all-dev
```

### **2️⃣ Build the Project**
Run the following commands:
```sh
mkdir -p build && cd build
cmake ..
make -j$(nproc)
```

This will generate:
- `./build/Market_Parser` (Main application)
- `./build/TestMarketDataServer` (Test server)
- `./build/TestMarketDataClient` (Test client)


## 📌 Running the Application
### **Start the Market Data Server**
```sh
./Market_Parser
```

### **Start a Client**
Run this in **another terminal**:
```sh
./Market_Parser_Client
```

The client should receive real-time market data from the server.


## 📌 Logging
All log messages (info & errors) are saved in **log.txt**.

```sh
tail -f log.txt
```


## 📌 Next Steps
1. 
