#pragma once

#include <iostream>
#include <string>
#include <fstream>
#include <mutex>


class Logger
{
public:
enum class LogLevel
{
    INFO,
    WARNING,
    ERROR
};

    // Delete copy construtor and assignment operator to force the single instance
    Logger(const Logger&)=delete;
    Logger& operator=(const Logger&) = delete;

    // Static method to return the single instance of the logger
    static Logger& getInstance()
    {
        static Logger instance;
        return instance;
    }


    void log(const std::string& message, LogLevel level);
    void setLogFile(const std::string& logFile);

private:
    /* data */
    std::ofstream m_logStream;
    std::mutex logMutex;

    // Private construtor to enforce singleton 
    Logger(/* args */)=default;
    ~Logger();

};

