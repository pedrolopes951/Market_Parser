#include "DataParser.hpp"
#include "Logger.hpp"
#include "BenchMark.hpp"


DataParser::DataParser(const std::string& timestamp, double price,uint32_t quatity):m_timestamp{timestamp},m_price{price},m_quantity{quatity}{}


std::vector<DataParser> ParsingFunctions::readCSV(const char* pathFileCSV)
{
    Timer timer;
    timer.start();
    std::vector<DataParser> marketData;
    marketData.reserve(10000000); // Reserve for 1M records
    // Read Mode
    std::ifstream file(pathFileCSV);
    
    
    if (!file.is_open())
    {
        Logger::getInstance().log("File not Open : " + std::string(pathFileCSV),Logger::LogLevel::ERROR);
        throw std::runtime_error("Unable to open file: " + std::string(pathFileCSV));
    }

    // Convert the file in to a bulk of chars to read it faster and
    std::vector<char> buffer(std::istreambuf_iterator<char>(file),{});
    std::istringstream filecontent(std::string(buffer.data(),buffer.size()));
    
    std::string line;

    // Skip header line
    std::getline(filecontent,line);
    Logger::getInstance().log("Header Line skipped sucefully" ,Logger::LogLevel::INFO);

    while(std::getline(filecontent,line))
    {
        std::stringstream ss(line);
        std::string timestamp;
        double price;
        uint32_t quantity;

        // Get first timespamp, skip comma, get prince, ignore coma and finnaly gets quantity 
        if(std::getline(ss,timestamp,',') && ss >> price && ss.ignore(1) && ss >> quantity)
        {
            marketData.emplace_back(std::move(timestamp),price,quantity);
        }
        else
        {
            Logger::getInstance().log("Bad Line: "+ line,Logger::LogLevel::WARNING);
        }

    }
    timer.end();
    timer.printTime();
    std::ostringstream logStream;
    logStream << "Successfully parsed " << std::to_string(marketData.size()) << " rows.";
    Logger::getInstance().log(logStream.str(), Logger::LogLevel::INFO);

    return marketData;
} 