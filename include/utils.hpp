#pragma once

#include "const.hpp"
#include "types.hpp"

#include <phosphor-logging/lg2.hpp>
#include <sdbusplus/bus.hpp>
#include <sdbusplus/exception.hpp>
#include <sstream>
#include <string>
#include <variant>

namespace panel
{
namespace utils
{
/**
 * @brief Read a single D-Bus property
 *
 * Generic helper that issues a org.freedesktop.DBus.Properties.Get call and
 * returns the result as the caller-supplied variant type T.
 *
 * @tparam T  std::variant specialisation that matches the property type.
 *
 * @param[in] service   - D-Bus service name.
 * @param[in] objPath   - D-Bus object path.
 * @param[in] interface - D-Bus interface name.
 * @param[in] property  - Property name.
 *
 * @return The property value as type T.  Returns a default-constructed T on
 *         any D-Bus error so that callers can handle absence gracefully.
 */
template <typename T>
T readBusProperty(const std::string& service, const std::string& objPath,
                  const std::string& interface,
                  const std::string& property) noexcept
{
    try
    {
        auto bus = sdbusplus::bus::new_default();
        auto method =
            bus.new_method_call(service.c_str(), objPath.c_str(),
                                "org.freedesktop.DBus.Properties", "Get");
        method.append(interface, property);

        auto reply = bus.call(method);

        T result{};
        reply.read(result);
        return result;
    }
    catch (const sdbusplus::exception::SdBusError& ex)
    {
        lg2::error(
            "D-Bus property read failed [{SERVICE} {PATH} {INTF} {PROP}]: "
            "{ERROR}",
            "SERVICE", service, "PATH", objPath, "INTF", interface, "PROP",
            property, "ERROR", ex.what());
        return T{};
    }
}

/**
 * @brief Convert a byte vector to an hex string
 *
 * Each byte is encoded as exactly two hex digits with no separator,
 * e.g. {0x50, 0x00, 0x10, 0x01} → "50001001".
 *
 * @param[in] bytes - Raw byte vector to convert.
 *
 * @return hex string, or empty string if @p bytes is empty.
 */
inline std::string bytesToHexString(const types::Binary& bytes) noexcept
{
    std::ostringstream oss;
    oss << std::setfill('0') << std::hex;
    for (const auto& byte : bytes)
    {
        oss << std::setw(2) << static_cast<int>(byte);
    }
    return oss.str();
}

/**
 * @brief Read the system IM keyword from D-Bus
 *
 * The API reads IM from dbus and read converts raw bytes into a printable hex
 * string
 *
 * @return IM value in hex string, or empty string on failure.
 */
inline std::string getSystemIM() noexcept
{
    auto res = readBusProperty<std::variant<types::Binary>>(
        constants::pimService, constants::systemInvPath,
        constants::vsbpInterface, constants::kwdIM);

    const auto* imBytes = std::get_if<types::Binary>(&res);
    if (imBytes == nullptr)
    {
        lg2::error("Unexpected type returned for IM keyword.");
        return {};
    }
    else if (imBytes->empty())
    {
        lg2::error("IM keyword read returned empty value.");
        return {};
    }

    return bytesToHexString(*imBytes);
}

} // namespace utils
} // namespace panel
