#include "MarketDataServer.hpp"
#include "Logger.hpp"
#include "DataParser.hpp"
#include <iostream>
#include <thread>
#include <vector>
#include <chrono>

void MarkerDataServer::StartServer(int port,const std::string& csvFile)
{
    io_service io; // Handle I/O operations
    tcp::acceptor acceptor(io, tcp::endpoint(tcp::v4(),8080)); // Listening sockets not for communication


    // Will listen for the request form the client
    while(true)
    {
        // creates a new socet to handle the client
        auto socket = std::make_shared<tcp::socket>(io);
        acceptor.accept(*socket);
        Logger::getInstance().log("Cliente Connected", Logger::LogLevel::INFO);
        std::thread([socket,csvFile]() { marketDataServer(socket, csvFile); }).detach();
    }
}

void MarkerDataServer::marketDataServer(std::shared_ptr<tcp::socket> socket,const std::string& csvFile)
{
    // Load market data from DataParser
    std::vector<DataParser> marketData = ParsingFunctions::readCSV(csvFile.c_str());

    try
    {
        for(const auto& entry: marketData)
        {
            // Format the message
            std::string message = entry.m_timestamp + "," + std::to_string(entry.m_price) + "," + std::to_string(entry.m_quantity) + "\n";
            
            // Send data over socket
            boost::system::error_code ignored_error;
            boost::asio::write(*socket,boost::asio::buffer(message), ignored_error);

            // Simulate real-time market updates
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
        }
    }
    catch(const std::exception& e)
    {
        Logger::getInstance().log("Market Data feed erorr:"+std::string(e.what()),Logger::LogLevel::ERROR);
        
    }
    
}
