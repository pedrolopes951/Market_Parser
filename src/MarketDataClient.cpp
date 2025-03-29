#include "MarketDataClient.hpp"
#include <iostream>
#include <sstream>
#include <iomanip>
#include <boost/asio.hpp>

namespace MarketDataClient
{
    void connectToServer(const std::string& serverAddress, int port) {
        try {
            // Create IO context
            boost::asio::io_context ioc;
            
            // Create a socket
            auto socket = std::make_shared<tcp::socket>(ioc);
            
            // Resolve the host
            tcp::resolver resolver(ioc);
            auto endpoints = resolver.resolve(serverAddress, std::to_string(port));
            
            // Connect to the server
            boost::asio::connect(*socket, endpoints);
            
            Logger::getInstance().log("Connected to market data server at " + 
                                     serverAddress + ":" + std::to_string(port), 
                                     Logger::LogLevel::INFO);
            
            // Handle market data
            handleMarketData(socket);
        }
        catch (const std::exception& e) {
            Logger::getInstance().log("Client error: " + std::string(e.what()), 
                                     Logger::LogLevel::ERROR);
        }
    }
    
    void handleMarketData(std::shared_ptr<tcp::socket> socket) {
        try {
            // Read data from the socket
            boost::asio::streambuf buffer;
            boost::asio::read_until(*socket, buffer, '\0');
            
            // Convert to string
            std::string data{std::istreambuf_iterator<char>(&buffer), {}};
            
            Logger::getInstance().log("Received " + std::to_string(data.size()) + 
                                     " bytes of market data", Logger::LogLevel::INFO);
            
            // Use our CSV parser to parse the data
            auto parser = ParserFactory::createCSVParser("memory_data.csv");
            
            // Create a stringstream from the received data
            std::istringstream ss(data);
            
            // Parse the CSV data
            // Note: We need to modify the parser to accept an input stream instead of file path
            // For now, we'll parse manually
            
            std::vector<MarketDataEntry> marketData;
            std::string line;
            
            // Skip header line
            std::getline(ss, line);
            
            // Parse each line
            while (std::getline(ss, line)) {
                std::stringstream lineStream(line);
                std::string timestamp;
                double open, high, low, close, volume;
                
                // Parse CSV line
                if (std::getline(lineStream, timestamp, ',') && 
                    lineStream >> open && lineStream.ignore(1) && 
                    lineStream >> high && lineStream.ignore(1) && 
                    lineStream >> low && lineStream.ignore(1) && 
                    lineStream >> close && lineStream.ignore(1) && 
                    lineStream >> volume) {
                    
                    marketData.emplace_back(timestamp, open, high, low, close, volume);
                }
                else {
                    Logger::getInstance().log("Bad line in received data: " + line, 
                                             Logger::LogLevel::WARNING);
                }
            }
            
            // Display the market data
            displayMarketData(marketData);
        }
        catch (const std::exception& e) {
            Logger::getInstance().log("Error handling market data: " + std::string(e.what()), 
                                     Logger::LogLevel::ERROR);
        }
    }
    
    void displayMarketData(const std::vector<MarketDataEntry>& data, size_t maxEntries) {
        if (data.empty()) {
            std::cout << "No market data received." << std::endl;
            return;
        }
        
        // Display the most recent entries (limited by maxEntries)
        size_t count = std::min(maxEntries, data.size());
        size_t startIdx = data.size() - count;
        
        // Print header
        std::cout << "\n===== MARKET DATA (Most Recent " << count << " Entries) =====\n";
        std::cout << std::left 
                  << std::setw(25) << "Timestamp" 
                  << std::setw(10) << "Open" 
                  << std::setw(10) << "High" 
                  << std::setw(10) << "Low" 
                  << std::setw(10) << "Close" 
                  << std::setw(10) << "Volume" 
                  << std::endl;
        
        std::cout << std::string(75, '-') << std::endl;
        
        // Print each entry
        for (size_t i = startIdx; i < data.size(); ++i) {
            const auto& entry = data[i];
            std::cout << std::left 
                      << std::setw(25) << entry.m_timestamp 
                      << std::setw(10) << entry.m_open 
                      << std::setw(10) << entry.m_high 
                      << std::setw(10) << entry.m_low 
                      << std::setw(10) << entry.m_close 
                      << std::setw(10) << entry.m_volume 
                      << std::endl;
        }
        
        std::cout << std::string(75, '-') << std::endl;
        std::cout << "Last updated: " << data.back().m_timestamp << std::endl;
    }
}