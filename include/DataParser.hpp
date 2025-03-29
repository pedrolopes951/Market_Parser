#pragma once
// Import Lib for file handling
#include <fstream>
#include <sstream>
// Import Data Structure to hold the market_data
#include <vector>
#include <cstdint>
#include <string>
#include <memory> // for the std::unique_prt

struct MarketDataEntry
{
    std::string m_timestamp;
    double m_open;
    double m_high;
    double m_low;
    double m_close;
    double m_volume;

    MarketDataEntry() = default;
    MarketDataEntry(const std::string &timestamp, double open, double high, double low, double close, double volume)
        : m_timestamp(timestamp), m_open(open), m_high(high), m_low(low), m_close(close), m_volume(volume) {}

    // Virutal destrocutor for proper clensing out
    ~MarketDataEntry() = default;
};

/// @brief Interface for Parsing Structs
class IDataParser
{
public:
    IDataParser() = default; // Tell compiler to generate it, with default
    virtual ~IDataParser() = default;

    virtual bool parseData() = 0; // Pure Virtual Data

    // Virutal getDta that can be ised for retrived polymorfic results
    virtual const std::vector<MarketDataEntry> &getData() const = 0;

    // Disable copying to avoid slicing problems with polimorphics behaviour
    // Slicing fyi is when a copy od a derived class object to a base class object, and it copyes only the base one (slicing it)
    // This way we are used to use pointers
    IDataParser(const IDataParser &) = delete;
    IDataParser &operator=(const IDataParser &) = delete;
};

class DataParserCSV : public IDataParser
{
public:
    explicit DataParserCSV(const std::string &CSVPath);
    virtual const std::vector<MarketDataEntry> &getData() const override;
    virtual bool parseData() override;

private:
    std::string m_CSVPath;
    std::vector<MarketDataEntry> m_data;
};

class DataParserJson : public IDataParser
{
public:
    explicit DataParserJson(const std::string &jsonContent);
    virtual const std::vector<MarketDataEntry> &getData() const override;
    virtual bool parseData() override;

private:
    std::string m_jsonContent;
    std::vector<MarketDataEntry> m_data;
};

/**
 * @brief Factory class for creating appropriate parsers
 */
class ParserFactory
{
public:
    // Creates a parser based on file extension or content type
    static std::unique_ptr<IDataParser> createParser(const std::string &source);

    // Specific factory methods
    static std::unique_ptr<IDataParser> createCSVParser(const std::string &filePath);
    static std::unique_ptr<IDataParser> createJSONParser(const std::string &jsonContent);
};

namespace ParsingFunctions
{

    // Function to parse the file locatedin the pathFileCSV and return a const object of the DataParserCSV
    std::vector<MarketDataEntry> readCSV(const char *pathFileCSV);

};
