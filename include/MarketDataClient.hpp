#pragma once
#include <string>
#include <boost/asio.hpp>
#include "Logger.hpp"
#include "DataParser.hpp"
#include <memory>

using namespace boost::asio;
using ip::tcp;

namespace MarkerDataClient
{
    void connectToServer(const std::string& serverAdress, int port);
    void marketDatahandler(std::shared_ptr<tcp::socket> socket);
};
