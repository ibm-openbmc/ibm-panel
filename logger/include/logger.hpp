#pragma once

#include <filesystem>
#include <iostream>
#include <list>
#include <map>
#include <source_location>
#include <sstream>

namespace Logger
{

enum Loglevel
{
    INFO,
    CRITICAL
};

// Maximum size of buffer is 8k bytes
const size_t maxBufferSize = 8000;
inline std::list<std::string> circularBuffer;

/**
 *@brief Appends data to the buffer
 *
 *This API is called to log message to the buffer.
 *It checks the log level and appends information like
 *line number, file name, timestamp to the message.
 *
 *@param[in] level - loglevel of the message
 *@param[in] message - Information to be logged
 *@param[in] location - object of the source_location class
 */
void logMessage(
    const Loglevel level, const std::string& message,
    const std::source_location& location = std::source_location::current());

/**
 *@brief Appends data to buffer with INFO as default Log Level
 *
 *@param[in] message - Information to be logged
 *@param[in] loaction - object of the source_location class
 */
void logMessage(
    const std::string& message,
    const std::source_location& location = std::source_location::current());

}; // namespace Logger
