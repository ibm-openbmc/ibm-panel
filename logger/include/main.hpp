#pragma once

#include <iostream>
#include <source_location>

/*
 * @brief Define API's to append log messages to a buffer
 */
namespace logging
{
void logMessage(std::string msg, const std::source_location& location);

} // namespace logging
