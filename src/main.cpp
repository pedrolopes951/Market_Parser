#include "MarketDataClient.hpp"
#include "MarketDataServer.hpp"
#include "Logger.hpp"
#include <thread>
#include <chrono>

int main()
{
    Logger::getInstance().setLogFile("log.txt");

    // Start server in a separate thread  111111111
    std::thread serverThread([]() {
        MarkerDataServer::StartServer(8080, std::string(DATA_FOLDER)+"/market_data_test.csv");
    });

    // Wait for the server to start  m
    std::this_thread::sleep_for(std::chrono::seconds(1));

    // Start client
    MarkerDataClient::connectToServer("127.0.0.1", 8080);

    serverThread.join(); // Keep main thread alive
    return 0;
}