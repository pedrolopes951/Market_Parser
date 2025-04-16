#include "MarketDataServer.hpp"
#include "Logger.hpp"
#include "BenchMark.hpp"
#include "DataParser.hpp"
#include <iostream>
#include <thread>
#include <vector>
#include <chrono>
#include <boost/beast.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/asio/ssl.hpp>
#include <nlohmann/json.hpp>
#include <sstream>

namespace beast = boost::beast;
namespace http = beast::http;
namespace net = boost::asio;
namespace ssl = net::ssl;
using tcp = net::ip::tcp;
using json = nlohmann::json;

namespace
{
    // Global cache of market data
    std::shared_ptr<MarketDataServer::DataCache> g_dataCache = std::make_shared<MarketDataServer::DataCache>();

    // Create SSL context for secure connections
    std::shared_ptr<ssl::context> createSSLContext()
    {
        auto ctx = std::make_shared<ssl::context>(ssl::context::tlsv12_client);
        ctx->set_verify_mode(ssl::verify_none);
        // ctx->set_default_verify_paths();
        return ctx;
    }
}

namespace MarketDataServer
{
    std::atomic<bool> g_shouldContinueFetching(false);

    // Implement DataCache methods
    void DataCache::updateData(const std::string &symbol, const std::vector<MarketDataEntry> &data)
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_cache[symbol] = data;
    }

    std::vector<MarketDataEntry> DataCache::getData(const std::string &symbol) const
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        auto it = m_cache.find(symbol);
        return (it != m_cache.end()) ? it->second : std::vector<MarketDataEntry>();
    }

    void StartServer(const ServerConfig &config)
    {
        try
        {
            Logger::getInstance().log("Starting Market Data Server on port " +
                                          std::to_string(config.port),
                                      Logger::LogLevel::INFO);

            // Create IO context
            net::io_context ioc;

            // Create the acceptor
            tcp::acceptor acceptor(ioc);
            tcp::endpoint endpoint(tcp::v4(), config.port);

            // Try to open and bind to the specified port
            boost::system::error_code ec;
            acceptor.open(endpoint.protocol(), ec);

            if (ec)
            {
                Logger::getInstance().log("Cannot open endpoint: " + ec.message(),
                                          Logger::LogLevel::ERROR);
                throw boost::system::system_error(ec);
            }

            // Set option to allow address reuse
            acceptor.set_option(tcp::acceptor::reuse_address(true));

            // Try to bind to the port
            acceptor.bind(endpoint, ec);

            if (ec)
            {
                Logger::getInstance().log("Cannot bind to port " +
                                              std::to_string(config.port) +
                                              ": " + ec.message(),
                                          Logger::LogLevel::WARNING);

                // Close the current acceptor
                acceptor.close();

                // Create a new endpoint with port 0 (system-assigned port)
                endpoint.port(0);

                // Create a new acceptor
                tcp::acceptor new_acceptor(ioc);
                new_acceptor.open(endpoint.protocol());
                new_acceptor.set_option(tcp::acceptor::reuse_address(true));
                new_acceptor.bind(endpoint);

                // Get the assigned port
                int actual_port = new_acceptor.local_endpoint().port();
                Logger::getInstance().log("Using alternative port: " +
                                              std::to_string(actual_port),
                                          Logger::LogLevel::INFO);

                // Start listening on the new acceptor
                new_acceptor.listen();

                Logger::getInstance().log("Server started. Listening on port " +
                                              std::to_string(actual_port),
                                          Logger::LogLevel::INFO);

                // Use the new acceptor for accepting connections
                accept_connections(new_acceptor);
            }
            else
            {
                // Start listening on the original port
                acceptor.listen();

                Logger::getInstance().log("Server started. Listening on port " +
                                              std::to_string(config.port),
                                          Logger::LogLevel::INFO);

                // Use the original acceptor for accepting connections
                accept_connections(acceptor);
            }
        }
        catch (const std::exception &e)
        {
            Logger::getInstance().log("Server error: " + std::string(e.what()),
                                      Logger::LogLevel::ERROR);
        }
    }

    // Helper function to accept connections
    void accept_connections(tcp::acceptor &acceptor)
    {
        // Accept connections in a loop
        while (true)
        {
            // Create a socket
            auto socket = std::make_shared<tcp::socket>(acceptor.get_executor());

            // Accept a connection
            acceptor.accept(*socket);

            Logger::getInstance().log("Client connected: " +
                                          socket->remote_endpoint().address().to_string(),
                                      Logger::LogLevel::INFO);

            // Handle the client in a separate thread
            std::thread clientThread(HandleClient, socket);
            clientThread.detach();
        }
    }

    void HandleClient(std::shared_ptr<tcp::socket> socket)
    {
        try
        {
            // Read client request for which symbol they want
            boost::asio::streambuf buffer;
            boost::asio::read_until(*socket, buffer, "\n");

            // Extract the symbol from the request
            std::string message(boost::asio::buffer_cast<const char *>(buffer.data()),
                                buffer.size());
            std::string symbol = "AAPL"; // Default to AAPL if no valid request

            // Parse the message to get the symbol (simple protocol: "GET SYMBOL\n")
            if (message.substr(0, 3) == "GET" && message.length() > 4)
            {
                symbol = message.substr(4, message.find('\n') - 4);
                // Trim whitespace
                symbol.erase(0, symbol.find_first_not_of(" \t"));
                symbol.erase(symbol.find_last_not_of(" \t\n\r") + 1);

                Logger::getInstance().log("Client requested symbol: " + symbol,
                                          Logger::LogLevel::INFO);
            }

            // Send the requested symbol's data
            SendMarketData(socket, symbol);
        }
        catch (const std::exception &e)
        {
            Logger::getInstance().log("Client handler error: " + std::string(e.what()),
                                      Logger::LogLevel::ERROR);
        }

        // Close the socket
        boost::system::error_code ec;
        socket->close(ec);
        if (ec)
        {
            Logger::getInstance().log("Error closing socket: " + ec.message(),
                                      Logger::LogLevel::WARNING);
        }

        Logger::getInstance().log("Client disconnected", Logger::LogLevel::INFO);
    }

    std::string FetchMarketData(const std::string &symbol, const std::string &apiKey)
    {
        Timer timer;
        timer.start();

        std::string response;

        try
        {
            // Host for Alpha Vantage API
            const std::string host = "www.alphavantage.co";

            // Set up I/O context and SSL context
            net::io_context ioc;
            auto ctx = createSSLContext();

            // These objects perform our I/O
            tcp::resolver resolver(ioc);
            ssl::stream<tcp::socket> stream(ioc, *ctx);

            // Look up the domain name
            auto const results = resolver.resolve(host, "443");

            // Make the connection
            net::connect(stream.next_layer(), results.begin(), results.end());

            if (!SSL_set_tlsext_host_name(stream.native_handle(), "www.alphavantage.co"))
            {
                throw boost::system::system_error(
                    ::ERR_get_error(),
                    boost::asio::error::get_ssl_category());
            }
            // Perform the SSL handshake
            try
            {
                stream.handshake(ssl::stream_base::client);
            }
            catch (const boost::system::system_error &e)
            {
                std::cerr << "SSL Handshake Error: " << e.what() << std::endl;
                std::cerr << "Error code: " << e.code() << std::endl;
                throw; // Re-throw to be caught by the outer catch
            }
            // Set up an HTTP GET request
            // Request intraday data with 1min interval
            std::string target = "/query?function=TIME_SERIES_INTRADAY&symbol=" + symbol +
                                 "&interval=1min&apikey=" + apiKey;

            http::request<http::string_body> req{http::verb::get, target, 11};
            req.set(http::field::host, host);
            req.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);

            // Send the HTTP request
            http::write(stream, req);

            // This buffer is used for reading
            beast::flat_buffer buffer;

            // Declare a container to hold the response
            http::response<http::dynamic_body> res;

            // Receive the HTTP response
            http::read(stream, buffer, res);

            // Convert to string
            response = beast::buffers_to_string(res.body().data());

            // Log the first 500 characters of the response for debugging
            std::string response_preview = response.substr(0, std::min<size_t>(500, response.size()));
            Logger::getInstance().log("API Response preview: " + response_preview + "...",
                                      Logger::LogLevel::INFO);

            // Gracefully close the stream
            beast::error_code ec;
            stream.shutdown(ec);
            if (ec == net::error::eof)
            {
                // Rationale: http://stackoverflow.com/questions/25587403/
                ec = {};
            }
            if (ec)
            {
                Logger::getInstance().log("SSL shutdown error: " + ec.message(),
                                          Logger::LogLevel::WARNING);
            }

            timer.end();
            timer.printTime();

            Logger::getInstance().log("Successfully fetched market data for " + symbol,
                                      Logger::LogLevel::INFO);
        }
        catch (const std::exception &e)
        {
            Logger::getInstance().log("Error fetching market data: " + std::string(e.what()),
                                      Logger::LogLevel::ERROR);
            timer.end();
        }

        return response;
    }
    // Fixed version with only the config parameter
    void DataUpdateTask(const ServerConfig config)
    {
        Logger &logger = Logger::getInstance();
        logger.log("Starting periodic market data fetch task", Logger::LogLevel::INFO);

        while (g_shouldContinueFetching)
        {
            for (const auto &symbol : config.symbols)
            {
                try
                {
                    logger.log("Fetching market data for " + symbol, Logger::LogLevel::INFO);

                    // Fetch data from API
                    std::string jsonResponse = FetchMarketData(symbol, config.apiKey);

                    bool apiDataProcessed = false;

                    if (!jsonResponse.empty())
                    {
                        auto jsonParser = ParserFactory::createJSONParser(jsonResponse);
                        if (jsonParser->parseData())
                        {
                            // Update cache with new data
                            g_dataCache->updateData(symbol, jsonParser->getData());
                            apiDataProcessed = true;

                            logger.log("Updated market data for " + symbol +
                                           ": " + std::to_string(jsonParser->getData().size()) +
                                           " entries",
                                       Logger::LogLevel::INFO);
                        }
                    }

                    // If API request failed or returned no data, fall back to CSV
                    if (!apiDataProcessed)
                    {
                        logger.log("API request failed or returned no data for " + symbol +
                                       ". Falling back to CSV data.",
                                   Logger::LogLevel::INFO);

                        // CSV fallback
                        auto csvParser = ParserFactory::createCSVParser(config.dataPath);
                        if (csvParser->parseData())
                        {
                            g_dataCache->updateData(symbol, csvParser->getData());

                            logger.log("Updated market data for " + symbol +
                                           " from CSV: " + std::to_string(csvParser->getData().size()) +
                                           " entries",
                                       Logger::LogLevel::INFO);
                        }
                        else
                        {
                            logger.log("Failed to load CSV fallback data for " + symbol,
                                       Logger::LogLevel::ERROR);
                        }
                    }
                }
                catch (const std::exception &e)
                {
                    logger.log("Error updating market data for " + symbol +
                                   ": " + std::string(e.what()),
                               Logger::LogLevel::ERROR);
                }
            }

            // Wait for next update interval
            std::this_thread::sleep_for(API_REFRESH_INTERVAL);
        }

        logger.log("Periodic market data fetch task stopped", Logger::LogLevel::INFO);
    }
    std::thread StartPeriodicFetching(const ServerConfig &config)
    {
        // Set the global flag
        g_shouldContinueFetching = true;

        // Start the thread with just the config parameter
        return std::thread(DataUpdateTask, config);
    }

    // Method to stop periodic fetching
    void StopPeriodicFetching()
    {
        g_shouldContinueFetching = false;
    }

    void SendMarketData(std::shared_ptr<tcp::socket> socket, const std::string &symbol)
    {
        // Use the global data cache instead of creating a new one
        std::vector<MarketDataEntry> data = g_dataCache->getData(symbol);

        try
        {
            if (data.empty())
            {
                // Send a proper error message instead of nothing
                std::string errorMsg = "ERROR: No data available for symbol: " + symbol + "\n";
                boost::asio::write(*socket, boost::asio::buffer(errorMsg));
                Logger::getInstance().log("No data available for " + symbol + ", sent error message",
                                          Logger::LogLevel::WARNING);
                return;
            }

            // Convert market data to CSV format for sending
            std::stringstream ss;
            ss << "timestamp,open,high,low,close,volume\n";

            for (const auto &entry : data)
            {
                ss << entry.m_timestamp << ","
                   << entry.m_open << ","
                   << entry.m_high << ","
                   << entry.m_low << ","
                   << entry.m_close << ","
                   << entry.m_volume << "\n";
            }

            std::string dataStr = ss.str();

            // First send a header with the data size
            std::string header = "DATA_SIZE:" + std::to_string(dataStr.size()) + "\n";
            boost::asio::write(*socket, boost::asio::buffer(header));

            // Then send the actual data
            boost::asio::write(*socket, boost::asio::buffer(dataStr));

            Logger::getInstance().log("Sent " + std::to_string(data.size()) +
                                          " market data entries to client",
                                      Logger::LogLevel::INFO);
        }
        catch (const std::exception &e)
        {
            Logger::getInstance().log("Error sending market data: " + std::string(e.what()),
                                      Logger::LogLevel::ERROR);
        }

        // Note: Do not close the socket here - let the client maintain the connection
    }

    std::vector<MarketDataEntry> GetLatestData(const std::string &symbol)
    {
        // Use the global data cache instead of creating a new one
        return g_dataCache->getData(symbol);
    }
}