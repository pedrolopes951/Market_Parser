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

            runClientInteractionLoop(socket);
        }
        catch (const std::exception &e)
        {
            Logger::getInstance().log("Client error: " + std::string(e.what()),
                                      Logger::LogLevel::ERROR);
        }
    }

    void runClientInteractionLoop(std::shared_ptr<tcp::socket> socket)
    {
        while (true)
        {
            try
            {
                // Show symbol selection menu
                std::string symbol = promptForSymbol();

                // Option to exit
                if (symbol == "exit" || symbol == "quit")
                {
                    std::cout << "Exiting client..." << std::endl;
                    break;
                }

                // Send request for the selected symbol
                std::string request = "GET " + symbol + "\n";
                boost::asio::write(*socket, boost::asio::buffer(request));

                // Receive and process data
                handleMarketData(socket);

                // Ask if user wants to continue
                std::cout << "\nDo you want to fetch more data? (yes/no): ";
                std::string continueChoice;
                std::getline(std::cin, continueChoice);

                // Convert to lowercase for case-insensitive comparison
                std::transform(continueChoice.begin(), continueChoice.end(),
                               continueChoice.begin(), ::tolower);

                // Break the loop if user doesn't want to continue
                if (continueChoice != "yes" && continueChoice != "y")
                {
                    std::cout << "Exiting client..." << std::endl;
                    break;
                }
            }
            catch (const std::exception &e)
            {
                Logger::getInstance().log("Client interaction error: " + std::string(e.what()),
                                          Logger::LogLevel::ERROR);

                // Ask if user wants to try again
                std::cout << "An error occurred. Do you want to try again? (yes/no): ";
                std::string retryChoice;
                std::getline(std::cin, retryChoice);

                // Convert to lowercase for case-insensitive comparison
                std::transform(retryChoice.begin(), retryChoice.end(),
                               retryChoice.begin(), ::tolower);

                // Break the loop if user doesn't want to retry
                if (retryChoice != "yes" && retryChoice != "y")
                {
                    std::cout << "Exiting client..." << std::endl;
                    break;
                }
            }
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
            Logger::getInstance().log("Waiting for data from server...", Logger::LogLevel::INFO);

            // Read the header line with data size
            boost::asio::streambuf header_buffer;
            boost::asio::read_until(*socket, header_buffer, "\n");

            std::string header_line(boost::asio::buffer_cast<const char *>(header_buffer.data()),
                                    header_buffer.size());

            // Check if it's an error message
            if (header_line.substr(0, 6) == "ERROR:")
            {
                Logger::getInstance().log(header_line, Logger::LogLevel::ERROR);
                return;
            }

            // Parse the data size from header
            size_t data_size = 0;
            if (header_line.substr(0, 10) == "DATA_SIZE:")
            {
                data_size = std::stoull(header_line.substr(10));
                Logger::getInstance().log("Expected data size: " + std::to_string(data_size) + " bytes",
                                          Logger::LogLevel::INFO);
            }
            else
            {
                Logger::getInstance().log("Invalid header format: " + header_line, Logger::LogLevel::ERROR);
                return;
            }

            // Read the exact amount of data
            boost::asio::streambuf data_buffer;
            boost::system::error_code error;
            size_t bytes_transferred = boost::asio::read(*socket, data_buffer,
                                                         boost::asio::transfer_exactly(data_size),
                                                         error);

            if (error)
            {
                Logger::getInstance().log("Error reading data: " + error.message(),
                                          Logger::LogLevel::ERROR);
                if (error != boost::asio::error::eof)
                {
                    throw boost::system::system_error(error);
                }
            }

            // Convert to string
            std::string data(boost::asio::buffer_cast<const char *>(data_buffer.data()),
                             bytes_transferred);

            Logger::getInstance().log("Received " + std::to_string(data.size()) + " bytes of data",
                                      Logger::LogLevel::INFO);

            // Parse directly from memory instead of using a temporary file
            auto parser = std::make_unique<DataParserCSV>(data);
            if (!parser->parseData())
            {
                Logger::getInstance().log("Failed to parse market data", Logger::LogLevel::ERROR);
                return;
            }

            // Get the parsed entries
            const std::vector<MarketDataEntry> &entries = parser->getData();
            Logger::getInstance().log("Parsed " + std::to_string(entries.size()) + " entries",
                                      Logger::LogLevel::INFO);

            if (entries.empty())
            {
                Logger::getInstance().log("No market data received", Logger::LogLevel::WARNING);
                return;
            }

            // Display filtered data
            Logger::getInstance().log("Displaying filtered data...", Logger::LogLevel::INFO);
            displayFilteredData(entries);
        }
        catch (std::exception &e)
        {
            Logger::getInstance().log("Error processing market data: " + std::string(e.what()),
                                      Logger::LogLevel::ERROR);
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