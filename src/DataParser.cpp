#include "DataParser.hpp"
#include "Logger.hpp"


DataParser::DataParser(const std::string& timestamp, double price,uint32_t quatity):m_timestamp{timestamp},m_price{price},m_quantity{quatity}{}


std::vector<DataParser> ParsingFunctions::readCSV(const char* pathFileCSV)
{
    std::vector<DataParser> marketData;
    // Read Mode
    std::ifstream file(pathFileCSV);
    
    
    if (!file.is_open())
    {
        Logger::getInstance().log("File not Open : " + std::string(pathFileCSV),Logger::LogLevel::ERROR);
        throw std::runtime_error("Unable to open file: " + std::string(pathFileCSV));
    }

    std::string line;

    // Skip header line
    std::getline(file,line);
    Logger::getInstance().log("Header Line skipped sucefully" ,Logger::LogLevel::INFO);


    while(std::getline(file,line))
    {
        std::stringstream ss(line);
        std::string timestamp;
        double price;
        uint32_t quantity;

        // Get first timespamp, skip comma, get prince, ignore coma and finnaly gets quantity 
        if(std::getline(ss,timestamp,',') && ss >> price && ss.ignore(1) && ss >> quantity)
        {
            marketData.emplace_back(timestamp,price,quantity);
        }
        else
        {
            Logger::getInstance().log("Bad Line: "+ line,Logger::LogLevel::WARNING);
        }

    }

    Logger::getInstance().log("Successfully parsed " + std::to_string(marketData.size()) + " rows.", Logger::LogLevel::INFO);

    return marketData;
} 