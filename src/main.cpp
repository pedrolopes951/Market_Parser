#include "MarketDataServer.hpp"
#include "MarketDataClient.hpp"
#include "Logger.hpp"
#include <fstream>

int main(int argc, char *argv[])
{
    // Set up logging
    Logger::getInstance().setLogFile("market_data_log.txt");

    // Check if running as client or server
    bool runAsClient = false;

    // Parse command line arguments
    for (int i = 1; i < argc; i++)
    {
        std::string arg = argv[i];
        if (arg == "--client" || arg == "-c")
        {
            runAsClient = true;
        }
    }

    if (runAsClient)
    {
        // Run as client
        std::cout << "Starting Market Data Client..." << std::endl;

        // Default server settings
        std::string serverAddress = "127.0.0.1";
        int port = MarketDataServer::DEFAULT_PORT;

        // Connect to server
        MarketDataClient::connectToServer(serverAddress, port);
    }
    else
    {
        // Run as server
        Logger::getInstance().log("Starting Alpha Vantage connector test...", Logger::LogLevel::INFO);

        // Configure server
        // In main.cpp where you configure the server
        MarketDataServer::ServerConfig config;
        config.apiKey = "UIDA0QOTLEQILCYZ"; // Your API key (with rate limits)
        config.symbols = {"AAPL", "MSFT", "GOOGL"};
        config.useCSV = true;                     // Enable CSV fallback
        config.dataPath = std::string(DATA_FOLDER) +  std::string("/market_data.csv"); // Path to your CSV file

        // Start periodic fetching (only once)
        std::thread fetchThread = MarketDataServer::StartPeriodicFetching(config);

        // Start server (this will block until server stops)
        MarketDataServer::StartServer(config);

        // Wait for fetch thread
        fetchThread.join();
    }

    return 0;
}