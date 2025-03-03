#pragma once
#include <memory>  // Include for shared_ptr
#include <boost/asio.hpp>
#include <string>
using namespace boost::asio;
using ip::tcp;


namespace MarkerDataServer
{
  void StartServer(int port,const std::string& csvFile);
  void marketDataServer(std::shared_ptr<tcp::socket> socket,const std::string& csvFile);
} 
