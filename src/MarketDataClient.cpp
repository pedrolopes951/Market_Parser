#include "MarketDataClient.hpp"

void MarkerDataClient::connectToServer(const std::string& serverAdress, int port)
{
    io_service io;
    auto socket = std::make_shared<tcp::socket>(io);
    try {

        socket->connect(tcp::endpoint(ip::address::from_string(serverAdress), port)); // Initiates a TCP handshake with the server.

        Logger::getInstance().log("Connected to Market Data Server",Logger::LogLevel::INFO);

        // Start therad to handle the receiving data

        std::thread(&MarkerDataClient::marketDatahandler, socket).detach();

        // Keep client running
        io.run();

    } catch (std::exception &e) {
       Logger::getInstance().log("Market Data Client Error: " + std::string(e.what()),Logger::LogLevel::ERROR);
    }
}

void MarkerDataClient::marketDatahandler(std::shared_ptr<tcp::socket> socket)
{
    std::vector<DataParser> marketData;
    boost::asio::streambuf buffer;
    std::istream input_stream(&buffer);
    std::string line;

    try {
        while (true) {
            // Read data from socket until newline
            boost::asio::read_until(*socket, buffer, "\n");
            std::getline(input_stream, line);

            // Log the received data
            Logger::getInstance().log("Received Market Data: " + line, Logger::LogLevel::INFO);

            // Parse CSV line into DataParser struct
            std::stringstream ss(line);
            std::string timestamp;
            double price;
            uint32_t quantity;

            if (std::getline(ss, timestamp, ',') && ss >> price && ss.ignore(1) && ss >> quantity) {
                marketData.emplace_back(timestamp, price, quantity);
            } else {
                Logger::getInstance().log("Invalid market data format: " + line, Logger::LogLevel::WARNING);
            }
        }
    } catch (const std::exception &e) {
        Logger::getInstance().log("Market Data Client Error: " + std::string(e.what()), Logger::LogLevel::ERROR);
        
    }

}
