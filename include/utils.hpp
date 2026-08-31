#pragma once

#include "const.hpp"
#include "types.hpp"

#include <string>

namespace panel
{
namespace utils
{
/**
 * @brief Read the system IM keyword from D-Bus
 *
 * The API reads IM from dbus and read convertes raw bytes into a printable hex
 * string.
 *
 * @return IM value as hex string, or empty string on failure.
 */
inline std::string getSystemIM() noexcept
{
    // ToDo Read IM Value from Dbus, on any failure return empty string.
    return std::string{};
}

} // namespace utils
} // namespace panel
