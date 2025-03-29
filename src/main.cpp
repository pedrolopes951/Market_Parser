#include "MarketDataServer.hpp"
#include "Logger.hpp"
#include <fstream>

int main(int argc, char* argv[]) {
    // Set up logging
    Logger::getInstance().setLogFile("market_data_log.txt");
    Logger::getInstance().log("Starting Alpha Vantage connector test...", Logger::LogLevel::INFO);
    
    // Set up basic configuration
    MarketDataServer::ServerConfig config;
    config.apiKey = "OGSUU6H80DHZNNCQ"; // Your API key
    config.symbols = {"AAPL"};          // Default to Apple stock
    
    if (argc > 1) {
        config.apiKey = argv[1];
    }
    if (argc > 2) {
        config.symbols.clear();
        config.symbols.push_back(argv[2]);
    }
    
    Logger::getInstance().log("Fetching market data for " + config.symbols[0] + " from Alpha Vantage...", 
                             Logger::LogLevel::INFO);
    
    // Test connection to Alpha Vantage
    std::string response = MarketDataServer::FetchMarketData(config.symbols[0], config.apiKey);
    
    if (!response.empty()) {
        Logger::getInstance().log("Successfully connected to Alpha Vantage API", Logger::LogLevel::INFO);
        
        // Save response to file
        std::ofstream outFile("api_response.json");
        if (outFile.is_open()) {
            outFile << response;
            outFile.close();
            Logger::getInstance().log("Full response saved to api_response.json", Logger::LogLevel::INFO);
        }
        
        // Parse the JSON response
        auto jsonParser = ParserFactory::createJSONParser(response);
        if (jsonParser->parseData()) {
            // Get the parsed data
            const auto& marketData = jsonParser->getData();
            
            // Log the number of parsed entries
            Logger::getInstance().log("Parsed " + std::to_string(marketData.size()) + 
                                    " market data entries", Logger::LogLevel::INFO);
            
            // Log a few sample entries for verification
            if (marketData.size() > 0) {
                std::stringstream ss;
                ss << "Sample entry: " << marketData[0].m_timestamp 
                   << " | Open: " << marketData[0].m_open
                   << " | Close: " << marketData[0].m_close;
                Logger::getInstance().log(ss.str(), Logger::LogLevel::INFO);
            }
        }
        else {
            Logger::getInstance().log("Failed to parse JSON response", Logger::LogLevel::ERROR);
        }
    } else {
        Logger::getInstance().log("Failed to connect to Alpha Vantage API", Logger::LogLevel::ERROR);
        return 1;
    }
    
    Logger::getInstance().log("Market data processing completed", Logger::LogLevel::INFO);
    return 0;
}