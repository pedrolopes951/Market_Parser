#pragma once
#include <string>
#include <boost/asio.hpp>
#include "Logger.hpp"
#include "DataParser.hpp"
#include <memory>

using namespace boost::asio;
using ip::tcp;

namespace MarketDataClient
{
    // Connect to a market data server
    void connectToServer(const std::string& serverAddress, int port);
    
    // Handle market data received from the server
    void handleMarketData(std::shared_ptr<tcp::socket> socket);
    
    // Display market data in the console
    void displayMarketData(const std::vector<MarketDataEntry>& data, size_t maxEntries = 10);

    // Display filtered market data
    void displayFilteredData(const std::vector<MarketDataEntry>& entries);

    // Allow user to prompt the symbol
    std::string promptForSymbol();
};