#include "DataParser.hpp"
#include "Logger.hpp"
#include "BenchMark.hpp"
#include <algorithm> // for std::min
#include <nlohmann/json.hpp>

// Used for Json parsing
using json = nlohmann::json;
constexpr const int NUMELEMENTS = 10000;

//----------------------------------------------
// DataParserCSV Implementation
//----------------------------------------------

DataParserCSV::DataParserCSV(const std::string& CSVPath)
    : m_CSVPath(CSVPath)
{
}

bool DataParserCSV::parseData()
{
    Timer timer;
    timer.start();
    
    // Clear any previously parsed data
    m_data.clear();
    
    // Reserve space for efficiency (avoid multiple reallocations)
    m_data.reserve(NUMELEMENTS); // Adjust based on expected data size
    
    try {
        // Open the file
        std::ifstream file(m_CSVPath);
        if (!file.is_open()) {
            Logger::getInstance().log("File not Open: " + m_CSVPath, Logger::LogLevel::ERROR);
            return false;
        }
        
        // Read the entire file into memory for faster processing
        std::vector<char> buffer(std::istreambuf_iterator<char>(file), {});
        std::istringstream fileContent(std::string(buffer.data(), buffer.size()));
        
        std::string line;
        
        // Skip header line
        std::getline(fileContent, line);
        Logger::getInstance().log("Header Line skipped successfully", Logger::LogLevel::INFO);
        
        // Process each line
        while (std::getline(fileContent, line)) {
            std::stringstream ss(line);
            std::string timestamp;
            double open, high, low, close, volume;
            
            // Parse the CSV line - expecting format: timestamp,open,high,low,close,volume
            if (std::getline(ss, timestamp, ',') && 
                ss >> open && ss.ignore(1) &&
                ss >> high && ss.ignore(1) &&
                ss >> low && ss.ignore(1) &&
                ss >> close && ss.ignore(1) &&
                ss >> volume) {
                
                m_data.emplace_back(timestamp, open, high, low, close, volume);
            }
            else {
                Logger::getInstance().log("Bad Line: " + line, Logger::LogLevel::WARNING);
            }
        }
        
        timer.end();
        timer.printTime();
        
        // Log successful parsing
        std::ostringstream logStream;
        logStream << "Successfully parsed " << m_data.size() << " rows from CSV.";
        Logger::getInstance().log(logStream.str(), Logger::LogLevel::INFO);
        
        return !m_data.empty();
        
    } catch (const std::exception& e) {
        Logger::getInstance().log("Error parsing CSV: " + std::string(e.what()), Logger::LogLevel::ERROR);
        timer.end();
        return false;
    }
}

const std::vector<MarketDataEntry>& DataParserCSV::getData() const
{
    return m_data;
}

//----------------------------------------------
// DataParserJson Implementation
//----------------------------------------------

DataParserJson::DataParserJson(const std::string& jsonContent)
    : m_jsonContent(jsonContent)
{
}

bool DataParserJson::parseData()
{
    Timer timer;
    timer.start();
    
    // Clear any previously parsed data
    m_data.clear();
    
    try {
        // Parse the JSON content
        json jsonData = json::parse(m_jsonContent);
        
        // Reserve space for better performance
        m_data.reserve(NUMELEMENTS); // Adjust based on expected data size
        
        // Handle different JSON formats
        
        // Alpha Vantage Time Series Daily format
        if (jsonData.contains("Time Series (Daily)")) {
            auto& timeSeries = jsonData["Time Series (Daily)"];
            
            for (auto it = timeSeries.begin(); it != timeSeries.end(); ++it) {
                const std::string& timestamp = it.key();
                auto& dataPoint = it.value();
                
                // Extract OHLCV data
                double open = std::stod(dataPoint["1. open"].get<std::string>());
                double high = std::stod(dataPoint["2. high"].get<std::string>());
                double low = std::stod(dataPoint["3. low"].get<std::string>());
                double close = std::stod(dataPoint["4. close"].get<std::string>());
                double volume = std::stod(dataPoint["5. volume"].get<std::string>());
                
                // Add to our market data vector
                m_data.emplace_back(timestamp, open, high, low, close, volume);
            }
        }
        // Alpha Vantage Intraday format
        else if (jsonData.contains("Time Series (1min)") || 
                 jsonData.contains("Time Series (5min)") ||
                 jsonData.contains("Time Series (15min)") ||
                 jsonData.contains("Time Series (30min)") ||
                 jsonData.contains("Time Series (60min)")) {
            
            // Find which time series we're working with
            std::string timeSeriesKey;
            for (const auto& key : {"Time Series (1min)", "Time Series (5min)", 
                                   "Time Series (15min)", "Time Series (30min)", 
                                   "Time Series (60min)"}) {
                if (jsonData.contains(key)) {
                    timeSeriesKey = key;
                    break;
                }
            }
            
            auto& timeSeries = jsonData[timeSeriesKey];
            
            for (auto it = timeSeries.begin(); it != timeSeries.end(); ++it) {
                const std::string& timestamp = it.key();
                auto& dataPoint = it.value();
                
                // Extract OHLCV data
                double open = std::stod(dataPoint["1. open"].get<std::string>());
                double high = std::stod(dataPoint["2. high"].get<std::string>());
                double low = std::stod(dataPoint["3. low"].get<std::string>());
                double close = std::stod(dataPoint["4. close"].get<std::string>());
                double volume = std::stod(dataPoint["5. volume"].get<std::string>());
                
                // Add to our market data vector
                m_data.emplace_back(timestamp, open, high, low, close, volume);
            }
        }
        // Custom or unknown format
        else {
            Logger::getInstance().log("JSON format not recognized as Alpha Vantage API response", 
                                    Logger::LogLevel::WARNING);
            
            // Try to parse as a simple array of OHLCV data
            if (jsonData.is_array()) {
                for (const auto& entry : jsonData) {
                    if (entry.contains("timestamp") && 
                        entry.contains("open") && 
                        entry.contains("high") && 
                        entry.contains("low") && 
                        entry.contains("close") && 
                        entry.contains("volume")) {
                        
                        m_data.emplace_back(
                            entry["timestamp"].get<std::string>(),
                            entry["open"].get<double>(),
                            entry["high"].get<double>(),
                            entry["low"].get<double>(),
                            entry["close"].get<double>(),
                            entry["volume"].get<double>()
                        );
                    }
                }
            }
            else {
                Logger::getInstance().log("Unable to parse JSON data in unknown format", 
                                        Logger::LogLevel::ERROR);
                timer.end();
                return false;
            }
        }
        
        timer.end();
        timer.printTime();
        
        // Log successful parsing
        std::ostringstream logStream;
        logStream << "Successfully parsed " << m_data.size() << " entries from JSON.";
        Logger::getInstance().log(logStream.str(), Logger::LogLevel::INFO);
        
        return !m_data.empty();
        
    } catch (const json::parse_error& e) {
        Logger::getInstance().log("JSON parsing error: " + std::string(e.what()), 
                                Logger::LogLevel::ERROR);
        timer.end();
        return false;
    } catch (const std::exception& e) {
        Logger::getInstance().log("Error during JSON parsing: " + std::string(e.what()), 
                                Logger::LogLevel::ERROR);
        timer.end();
        return false;
    }
}

const std::vector<MarketDataEntry>& DataParserJson::getData() const
{
    return m_data;
}

//----------------------------------------------
// ParserFactory Implementation
//----------------------------------------------

std::unique_ptr<IDataParser> ParserFactory::createParser(const std::string& source)
{
    // Check file extension using C++17 compatible methods
    // For CSV files
    if (source.size() >= 4 && 
        (source.compare(source.size() - 4, 4, ".csv") == 0 || 
         source.compare(source.size() - 4, 4, ".CSV") == 0)) {
        return createCSVParser(source);
    }
    // For JSON files or JSON content
    else if ((source.size() > 0 && source[0] == '{') || // Starts with '{'
             (source.size() >= 5 && source.compare(source.size() - 5, 5, ".json") == 0) ||
             (source.size() >= 5 && source.compare(source.size() - 5, 5, ".JSON") == 0)) {
        return createJSONParser(source);
    }
    else {
        Logger::getInstance().log("Unknown data format: " + source, Logger::LogLevel::WARNING);
        // Default to CSV parser
        return createCSVParser(source);
    }
}

std::unique_ptr<IDataParser> ParserFactory::createCSVParser(const std::string& filePath)
{
    return std::make_unique<DataParserCSV>(filePath);
}

std::unique_ptr<IDataParser> ParserFactory::createJSONParser(const std::string& jsonContent)
{
    return std::make_unique<DataParserJson>(jsonContent);
}



//----------------------------------------------
// Legacy function implementation
//----------------------------------------------

namespace ParsingFunctions {
    std::vector<MarketDataEntry> readCSV(const char* pathFileCSV)
    {
        auto parser = ParserFactory::createCSVParser(pathFileCSV);
        if (parser->parseData()) {
            return parser->getData();
        }
        return {};
    }
}