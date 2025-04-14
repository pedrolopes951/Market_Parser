#include "MarketDataClient.hpp"
#include <iostream>
#include <sstream>
#include <iomanip>
#include <boost/asio.hpp>

namespace MarketDataClient
{

    void connectToServer(const std::string &serverAddress, int port)
    {
        try
        {
            boost::asio::io_context io_context;
            auto socket = std::make_shared<tcp::socket>(io_context);

            // Resolve the server address
            tcp::resolver resolver(io_context);
            auto endpoints = resolver.resolve(serverAddress, std::to_string(port));

            // Connect to the server
            boost::asio::connect(*socket, endpoints);

            std::cout << "Connected to market data server at " << serverAddress << ":" << port << std::endl;

            // Show symbol selection menu
            std::string symbol = promptForSymbol();

            // Send request for the selected symbol
            std::string request = "GET " + symbol + "\n";
            boost::asio::write(*socket, boost::asio::buffer(request));

            // Receive and process data
            handleMarketData(socket);
        }
        catch (const std::exception &e)
        {
            Logger::getInstance().log("Client error: " + std::string(e.what()),
                                      Logger::LogLevel::ERROR);
        }
    }

    std::string promptForSymbol()
    {
        std::cout << "\n=== Market Data Client ===\n";
        std::cout << "Available symbols: AAPL, MSFT, GOOGL\n";
        std::cout << "Enter symbol: ";

        std::string symbol;
        std::getline(std::cin, symbol);

        // Default to AAPL if empty
        if (symbol.empty())
        {
            symbol = "AAPL";
            std::cout << "Using default symbol: AAPL\n";
        }

        return symbol;
    }

    void handleMarketData(std::shared_ptr<tcp::socket> socket)
    {
        try
        {
            std::cout << "Waiting for data from server..." << std::endl;

            // Buffer for incoming data
            boost::asio::streambuf buffer;

            // Read the entire response
            boost::system::error_code error;
            std::cout << "Reading data..." << std::endl;
            boost::asio::read(*socket, buffer, boost::asio::transfer_all(), error);

            if (error && error != boost::asio::error::eof)
            {
                std::cout << "Error reading data: " << error.message() << std::endl;
                throw boost::system::system_error(error);
            }

            // Convert to string
            std::string data(boost::asio::buffer_cast<const char *>(buffer.data()),
                             buffer.size());

            std::cout << "Received " << data.size() << " bytes of data" << std::endl;

            // Create a temporary file to use with the existing parser
            std::string tempFileName = "temp_market_data.csv";
            {
                std::ofstream tempFile(tempFileName);
                tempFile << data;
                std::cout << "Saved data to temporary file" << std::endl;
            }

            // Use the factory to create the appropriate parser
            std::cout << "Creating parser..." << std::endl;
            auto parser = ParserFactory::createCSVParser(tempFileName);

            // Parse the data
            std::cout << "Parsing data..." << std::endl;
            if (!parser->parseData())
            {
                std::cout << "Failed to parse market data" << std::endl;
                return;
            }

            // Get the parsed entries
            const std::vector<MarketDataEntry> &entries = parser->getData();
            std::cout << "Parsed " << entries.size() << " entries" << std::endl;

            if (entries.empty())
            {
                std::cout << "No market data received." << std::endl;
                return;
            }

            // Apply filters and display
            std::cout  << "Displaying filtered data..." << std::endl;
            displayFilteredData(entries);

            // Clean up temporary file
            std::remove(tempFileName.c_str());
        }
        catch (std::exception &e)
        {
            std::cerr << "Error processing market data: " << e.what() << std::endl;
        }
    }

    void displayFilteredData(const std::vector<MarketDataEntry> &entries)
    {
        // Show filtering options
        std::cout << "\n=== Filter Options ===\n";
        
        std::cout << "1. Show all data\n";
        std::cout << "2. Show last N entries\n";
        std::cout << "3. Show entries with price > X\n";
        std::cout << "Enter choice (default: 1): ";

        std::string choice;
        std::getline(std::cin, choice);

        if (choice.empty() || choice == "1")
        {
            // Show all entries
            displayMarketData(entries);
        }
        else if (choice == "2")
        {
            // Show last N entries
            std::cout << "How many entries to show? ";
            std::string countStr;
            std::getline(std::cin, countStr);
            size_t count = 10; // Default
            try
            {
                count = std::stoi(countStr);
            }
            catch (...)
            {
                std::cout << "Invalid number, using default: 10" << std::endl;
            }

            displayMarketData(entries, std::min(count, entries.size()));
        }
        else if (choice == "3")
        {
            // Filter by price
            std::cout << "Minimum price threshold: ";
            std::string priceStr;
            std::getline(std::cin, priceStr);
            double priceThreshold = 0.0;
            try
            {
                priceThreshold = std::stod(priceStr);
            }
            catch (...)
            {
                std::cout << "Invalid price, using default: 0.0" << std::endl;
            }

            std::vector<MarketDataEntry> filtered;
            for (const auto &entry : entries)
            {
                if (entry.m_close > priceThreshold)
                {
                    filtered.push_back(entry);
                }
            }

            std::cout << "Found " << filtered.size() << " entries with price > "
                      << priceThreshold << std::endl;
            displayMarketData(filtered);
        }
        else
        {
            std::cout << "Invalid choice, showing all data." << std::endl;
            displayMarketData(entries);
        }
    }

    void displayMarketData(const std::vector<MarketDataEntry> &data, size_t maxEntries)
    {
        if (data.empty())
        {
            std::cout << "No market data received." << std::endl;
            return;
        }

        // Calculate max entries to display
        size_t numEntries = std::min(maxEntries, data.size());

        // Calculate the starting index (for showing last N entries)
        size_t startIndex = data.size() - numEntries;

        // Table header
        std::cout << "\n=== Market Data (" << numEntries << " entries) ===\n";
        std::cout << std::left << std::setw(25) << "Timestamp"
                  << std::setw(10) << "Open"
                  << std::setw(10) << "High"
                  << std::setw(10) << "Low"
                  << std::setw(10) << "Close"
                  << "Volume" << std::endl;
        std::cout << std::string(70, '-') << std::endl;

        // Table data
        for (size_t i = startIndex; i < data.size(); ++i)
        {
            const auto &entry = data[i];
            std::cout << std::left << std::setw(25) << entry.m_timestamp
                      << std::fixed << std::setprecision(2)
                      << std::setw(10) << entry.m_open
                      << std::setw(10) << entry.m_high
                      << std::setw(10) << entry.m_low
                      << std::setw(10) << entry.m_close
                      << std::setprecision(0) << entry.m_volume << std::endl;
        }

        std::cout << std::string(70, '-') << std::endl;
    }
}