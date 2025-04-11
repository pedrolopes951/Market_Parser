#include "MarketDataServer.hpp"
#include "Logger.hpp"
#include <fstream>

int main(int argc, char *argv[])
{
    // Set up logging
    Logger::getInstance().setLogFile("market_data_log.txt");
    Logger::getInstance().log("Starting Alpha Vantage connector test...", Logger::LogLevel::INFO);

    // Set up basic configuration
    MarketDataServer::ServerConfig config;
    config.apiKey = "OGSUU6H80DHZNNCQ"; // Your API key
    config.symbols = {"AAPL", "MSFT", "GOOGL"};

    if (argc > 1)
    {
        config.apiKey = argv[1];
    }
    if (argc > 2)
    {
        config.symbols.clear();
        config.symbols.push_back(argv[2]);
    }

    // Start periodic fetching
    std::thread fetchThread = MarketDataServer::StartPeriodicFetching(config);
    
    // Note: StartServer will not return until the server is stopped,
    // so the join() below will not be reached unless StartServer exits
    MarketDataServer::StartServer(config);
    
    // Wait for the fetch thread to finish
    fetchThread.join();

    return 0;
}