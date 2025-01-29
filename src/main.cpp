#include <iostream>
#include "Logger.hpp"
#include "DataParser.hpp"

int main()
{
    const std::string csvFile = std::string(DATA_FOLDER) + "/market_data_test.csv";

    Logger::getInstance().setLogFile("log.txt");

    try
    {
        auto data = ParsingFunctions::readCSV(csvFile.c_str());
        
        //Print values to the console to check
        for (const auto& entry : data)
        {
            std::cout << "Timestamp :" << entry.m_timestamp
                      << ",Price :" << entry.m_price
                      << ",Quatity :" << entry.m_quantity
                      << std::endl;
        }
        Logger::getInstance().log("Program completed successfully.", Logger::LogLevel::INFO);

        
    }
    catch(const std::exception& e)
    {
        Logger::getInstance().log(e.what(), Logger::LogLevel::ERROR);
    }
    
    return 0;
}