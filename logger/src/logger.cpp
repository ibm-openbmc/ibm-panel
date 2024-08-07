#include "logger.hpp"

#include <ctime>
#include <fstream>

namespace Logger
{

static std::map<Loglevel, std::string> logLevelMap = {
    {Loglevel::INFO, "INFO"}, {Loglevel::CRITICAL, "CRITICAL"}};

size_t byteCount = 0;
const std::string logFilePath = "/var/lib/vpd/panellog.log";

/**
 * @brief Generates a timestamp string
 *
 * Generates a timestamp string representing the current time
 * in the format "YYYY-MM-DD HH:MM:SS"
 *
 * @return A string containing the formatted timestamp
 */
static std::string timestamp()
{
    time_t t = time(nullptr);
    tm Time{};
    gmtime_r(&t, &Time);
    std::string timeStamp;
    timeStamp.resize(30, '\0');
    size_t currentTime = strftime(timeStamp.data(), timeStamp.size(),
                                  "%Y-%m-%d %H:%M:%S", &Time);
    timeStamp.resize(currentTime);
    return timeStamp;
}

void logMessage(const Loglevel level, const std::string& message,
                const std::source_location& location)
{
    std::string tmpString =
        timestamp() + " [" + logLevelMap[level] + "]" + " : " +
        std::filesystem::path(location.file_name()).filename().string() + ":" +
        std::to_string(location.line()) + " - " + message + "\n";
    while (byteCount + tmpString.size() > maxBufferSize)
    {
        byteCount = byteCount - (*circularBuffer.begin()).size();
        circularBuffer.erase(circularBuffer.begin());
    }
    if (Loglevel::INFO == level)
    {
        circularBuffer.push_back(tmpString);
        byteCount = byteCount + tmpString.size();
    }
}

void logMessage(const std::string& message,
                const std::source_location& location)
{
    logMessage(Loglevel::INFO, message, location);
}

void fetchNLogs(size_t noOfLogs)
{
    try
    {
        std::fstream logfile(logFilePath, std::ios::trunc | std::ios::out);
        if (!logfile.is_open())
        {
            throw std::runtime_error("Error opening " + logFilePath + "file");
        }

        if (noOfLogs > circularBuffer.size())
        {
            noOfLogs = circularBuffer.size();
        }

        auto iterator = circularBuffer.begin();
        std::advance(iterator, circularBuffer.size() - noOfLogs);
        while (iterator != circularBuffer.end())
        {
            logfile << *iterator;
            iterator++;
        }

        logfile.close();
    }
    catch (const std::exception& e)
    {
        std::cerr << "Exception: " << e.what() << std::endl;
    }
}

} // namespace Logger
