#include "logger.hpp"

#include <filesystem>
#include <iostream>
#include <sstream>
#include <string>

namespace logging
{
std::ostringstream logMsg;
void logMessage(std::string message, const std::source_location& location)
{
    logMsg << "File: "
           << std::filesystem::path(location.file_name()).filename().string()
           << " line: " << location.line() << " " << message << "\n";
}

} // namespace logging
