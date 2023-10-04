#include "../include/main.hpp"

#include <iostream>
#include <sstream>
#include <string>

namespace logging
{

void logMessage(std::string msg, const std::source_location& location)
{
    std::ostringstream logMsg;
    logMsg << "File: " << location.file_name() << "line: " << location.line()
           << " " << msg << "\n";
}

} // namespace logging
