#pragma once

#include <iostream>
#include <source_location>

namespace logging
{

/**
 *@brief An API to append log messages to a buffer
 *This API is called to log message. It also appends the
 *file name and line number of the mesaage.
 *
 *param[in] - Information that we want to log
 *param[in] - Object of source_location class
 */
void logMessage(std::string message, const std::source_location& location =
                                         std::source_location::current());

} // namespace logging
