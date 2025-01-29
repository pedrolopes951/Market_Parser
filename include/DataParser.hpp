#pragma once
// Import Lib for file handling
#include <fstream>
#include <sstream>
// Import Data Structure to hold the market_data
#include <vector>
#include <string>



struct DataParser
{
    /*
    DS to hold the market data 
    */
    std::string m_timestamp;
    double m_price;
    uint32_t m_quantity; // Might need to adjust maybe in the future the size
    DataParser(const std::string& timestamp, double price,uint32_t quatity);
};
namespace ParsingFunctions
{

// Function to parse the file locatedin the pathFileCSV and return a const object of the DataParser
std::vector<DataParser> readCSV(const char* pathFileCSV);

};



