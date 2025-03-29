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
    // Anonymous namespace (with no name) is a C++ feature that limits the scope of its contents to the current file only
    // It is like using Static for the cpp unit
    // Global cache of market data
    std::map<std::string, std::vector<MarketDataEntry>> g_marketDataCache;
    std::mutex g_cacheMutex;

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
    void StartServer(const ServerConfig &config)
    {
        try
        {
            Logger::getInstance().log("Starting Market Data Server on port " +
                                          std::to_string(config.port),
                                      Logger::LogLevel::INFO);

            // Create IO context
            net::io_context ioc;

            // Create and bind the acceptor
            tcp::acceptor acceptor(ioc, tcp::endpoint(tcp::v4(), config.port));

            Logger::getInstance().log("Server started. Listening on port " +
                                          std::to_string(config.port),
                                      Logger::LogLevel::INFO);

            // Start the background data update thread
            std::thread updateThread(DataUpdateTask, config);
            updateThread.detach(); // Let it run independently

            // Accept connections in a loop
            while (true)
            {
                // Create a socket
                auto socket = std::make_shared<tcp::socket>(ioc);

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
        catch (const std::exception &e)
        {
            Logger::getInstance().log("Server error: " + std::string(e.what()),
                                      Logger::LogLevel::ERROR);
        }
    }

    void HandleClient(std::shared_ptr<tcp::socket> socket)
    {
        try
        {
            // For now, default to sending AAPL data
            SendMarketData(socket, "AAPL");
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

    void DataUpdateTask(const ServerConfig &config)
    {
        Logger::getInstance().log("Starting market data update task", Logger::LogLevel::INFO);

        // Load initial data from CSV if specified
        if (!config.dataPath.empty() && config.useCSV)
        {
            try
            {
                auto csvParser = ParserFactory::createCSVParser(config.dataPath);
                if (csvParser->parseData())
                {
                    std::lock_guard<std::mutex> lock(g_cacheMutex);
                    // Default to AAPL for the CSV data
                    g_marketDataCache["AAPL"] = csvParser->getData();
                    Logger::getInstance().log("Loaded initial data from " + config.dataPath,
                                              Logger::LogLevel::INFO);
                }
            }
            catch (const std::exception &e)
            {
                Logger::getInstance().log("Error loading initial data: " + std::string(e.what()),
                                          Logger::LogLevel::ERROR);
            }
        }

        // Continuously update market data
        while (true)
        {
            for (const auto &symbol : config.symbols)
            {
                try
                {
                    // Fetch data from API
                    std::string jsonResponse = FetchMarketData(symbol, config.apiKey);

                    if (!jsonResponse.empty())
                    {
                        // Parse the JSON response
                        auto jsonParser = ParserFactory::createJSONParser(jsonResponse);
                        if (jsonParser->parseData())
                        {
                            // Update the cache
                            std::lock_guard<std::mutex> lock(g_cacheMutex);
                            g_marketDataCache[symbol] = jsonParser->getData();

                            Logger::getInstance().log("Updated market data for " + symbol +
                                                          ": " + std::to_string(jsonParser->getData().size()) +
                                                          " entries",
                                                      Logger::LogLevel::INFO);
                        }
                    }
                }
                catch (const std::exception &e)
                {
                    Logger::getInstance().log("Error updating market data for " + symbol +
                                                  ": " + std::string(e.what()),
                                              Logger::LogLevel::ERROR);
                }
            }

            // Wait for the next update interval
            std::this_thread::sleep_for(API_REFRESH_INTERVAL);
        }
    }

    void SendMarketData(std::shared_ptr<tcp::socket> socket, const std::string &symbol)
    {
        std::vector<MarketDataEntry> data;

        // Get the data from the cache
        {
            std::lock_guard<std::mutex> lock(g_cacheMutex);
            auto it = g_marketDataCache.find(symbol);
            if (it != g_marketDataCache.end())
            {
                data = it->second;
            }
        }

        if (data.empty())
        {
            Logger::getInstance().log("No data available for " + symbol, Logger::LogLevel::WARNING);
            return;
        }

        try
        {
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
    }

    std::vector<MarketDataEntry> GetLatestData(const std::string &symbol)
    {
        std::lock_guard<std::mutex> lock(g_cacheMutex);
        auto it = g_marketDataCache.find(symbol);
        if (it != g_marketDataCache.end())
        {
            return it->second;
        }
        return {};
    }

    // void fetchMarketData()
    // {
    //     try
    //     {
    //         io_context io;
    //         ssl::context ctx(ssl::context::tlsv12_client); // Create SSL context, creates the riles for creating a secure connection

    //         // Set SSL context options (no_sslv2 and no_sslv3) which is important
    //         ctx.set_options(ssl::context::default_workarounds | ssl::context::no_sslv2 | ssl::context::no_sslv3);

    //         // Create HTTPS connection
    //         tcp::resolver resolver(io); // TCP is for enebale the data transmisstion between the client and server
    //         ssl::stream<tcp::socket> stream(io, ctx); // Secure Socket to communicate with the API

    //         // Set SNI Hostname (many hosts need this to handshake successfully)
    //         if(! SSL_set_tlsext_host_name(stream.native_handle(), "www.alphavantage.co"))
    //         {
    //             throw boost::system::system_error(
    //                 ::ERR_get_error(),
    //                 boost::asio::error::get_ssl_category());
    //         }

    //         // Resolve the host
    //         auto const results = resolver.resolve("www.alphavantage.co", "443");
    //         boost::asio::connect(stream.next_layer(), results.begin(), results.end());

    //         // Set the expected hostname in the peer certificate for verification
    //         stream.set_verify_callback(ssl::rfc2818_verification("www.alphavantage.co"));

    //         // Perform TLS handshake to make the TCp connection secure
    //         // Back and forth between client and server to proof safe connectinos
    //         stream.handshake(ssl::stream_base::client);

    //         Logger::getInstance().log("Connected to Alpha Vantage API over HTTPS", Logger::LogLevel::INFO);

    //         // Construct the HTTP request
    //         // Fetches 1-minute interval price data for AAPL (Apple stock)
    //         // Uses API_KEY to authenticate with the Alpha Vantage API.
    //         http::request<http::string_body> req(http::verb::get, "/query?function=TIME_SERIES_INTRADAY&symbol=AAPL&interval=1min&apikey=" + string(API_KEY), 11);
    //         req.set(http::field::host, "www.alphavantage.co");
    //         req.set(http::field::user_agent, "Client");

    //         // Send the request
    //         http::write(stream, req);

    //         // Read the response
    //         boost::beast::flat_buffer buffer;
    //         http::response<http::dynamic_body> res; // Store the HTTP response
    //         http::read(stream, buffer, res);

    //         // Convert response body to string
    //         std::ostringstream response_data;
    //         response_data << boost::beast::buffers_to_string(res.body().data());

    //         Logger::getInstance().log("Market Data Response: " + response_data.str(), Logger::LogLevel::INFO);

    //         // Close the connection
    //         boost::system::error_code ec;
    //         stream.shutdown(ec);
    //         if (ec == boost::asio::error::eof)
    //         {
    //             // Rationale:
    //             // http://stackoverflow.com/questions/25587403/boost-asio-ssl-async-shutdown-always-finishes-with-an-error
    //             ec.assign(0, ec.category());
    //         }
    //         if (ec)
    //         {
    //             Logger::getInstance().log("Shutdown failed: " + ec.message(), Logger::LogLevel::ERROR);
    //         }
    //     }
    //     catch (const std::exception &e)
    //     {
    //         Logger::getInstance().log("Market Data Fetch Error: " + string(e.what()), Logger::LogLevel::ERROR);
    //     }
    // }
}
