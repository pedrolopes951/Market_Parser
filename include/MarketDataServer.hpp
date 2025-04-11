#pragma once
#include <memory>
#include "DataParser.hpp"
#include <boost/asio.hpp>
#include <utility>
#include <string>
using namespace boost::asio;
using ip::tcp;

// constexpr const char* API_KEY = "OGSUU6H80DHZNNCQ";
// constexpr const char* BASE_URL = "https://www.alphavantage.co/query";

namespace MarketDataServer
{

  // Constants
  constexpr int DEFAULT_PORT = 8080;
  constexpr auto API_REFRESH_INTERVAL = std::chrono::seconds(60); // Fetch data every 1 second

  struct ServerConfig
  {
    int port = DEFAULT_PORT;
    std::string apiKey;
    std::vector<std::string> symbols = {"APPL"};
    bool useCSV = false;                                                       // By default dont use CSV
    std::string dataPath = std::string(DATA_FOLDER) + "/market_data_test.csv"; // Optional falback to CSV path
  };

  class DataCache
  {
  public:
    void updateData(const std::string &symbol, const std::vector<MarketDataEntry> &data);
    std::vector<MarketDataEntry> getData(const std::string &symbol) const;

  private:
    std::unordered_map<std::string, std::vector<MarketDataEntry>> m_cache;
    mutable std::mutex m_mutex;
  };

  // Start the server with the given configuration
  void StartServer(const ServerConfig& config);

  // Handler for a single client connections
  void HandleClient(std::shared_ptr<tcp::socket> socket);

  // Fetch data from Alpha Vantage API
 std::string FetchMarketData(const std::string& symbol, const std::string& apiKey);

  void DataUpdateTask(const ServerConfig config);

  // Send market data to a client
  void SendMarketData(std::shared_ptr<tcp::socket> socket, const std::string &symbol);

  // Method for Startting periodic fetching
  std::thread StartPeriodicFetching(const ServerConfig& config);

  // Method to stop periodic fetching
  void StopPeriodicFetching();

  // Get the latest data for a symbol
 std::vector<MarketDataEntry> GetLatestData(const std::string& symbol);

  /// @brief
  /// Establish HTTPS connection
  /// Sending HTTP GET request to fwtch real-time makert data


  /// Reading the server responde and log market Data
  /// Propoerly Handling errors and closing the connection
  // void fetchMarketData();

}
