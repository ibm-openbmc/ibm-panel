#pragma once

#include "const.hpp"
#include "error_codes.hpp"
#include "types.hpp"

#include <expected>
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
 * @brief API to get error code message.
 *
 * @param[in] errCode - error code.
 *
 * @return Error message set for that error code. Otherwise empty
 * string.
 */
inline std::string getErrCodeMsg(const uint16_t& errCode) noexcept
{
    if (errorCodeMap.find(errCode) != errorCodeMap.end())
    {
        return errorCodeMap.at(errCode);
    }

    return std::format("Unknown error code [{}].", errCode);
}

/**
 * @brief An API to read property from Dbus.
 *
 * Generic helper that issues a org.freedesktop.DBus.Properties.Get call to
 * read the D-Bus property value.
 *
 * @param[in] service   - D-Bus service name.
 * @param[in] objPath   - D-Bus object path.
 * @param[in] interface - D-Bus interface name.
 * @param[in] property  - Property name.
 *
 * @return - Value read from Dbus, if success.
 *           corresponding error code is returned in failure case.
 */
inline std::expected<types::DbusVariantType, error_code>
    readDbusProperty(const std::string& service, const std::string& objectPath,
                     const std::string& interface,
                     const std::string& property) noexcept
{
    types::DbusVariantType propertyValue;

    // Mandatory fields to make a read dbus call.
    if (service.empty() || objectPath.empty() || interface.empty() ||
        property.empty())
    {
        lg2::error("One of the parameter to make Dbus read call is empty.");
        return std::unexpected(error_code::INVALID_INPUT_PARAMETER);
    }

    try
    {
        auto bus = sdbusplus::bus::new_default();
        auto method =
            bus.new_method_call(service.c_str(), objectPath.c_str(),
                                "org.freedesktop.DBus.Properties", "Get");
        method.append(interface, property);

        auto result = bus.call(method);
        result.read(propertyValue);
        return propertyValue;
    }
    catch (const std::exception& ex)
    {
        lg2::error(
            "D-Bus property read failed [{SERVICE} {PATH} {INTF} {PROP}]: "
            "{ERROR}",
            "SERVICE", service, "PATH", objectPath, "INTF", interface, "PROP",
            property, "ERROR", ex);
        return std::unexpected(error_code::DBUS_FAILURE);
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
 * @return hex string on success, corresponding error code is returned on
 * failure.
 */
inline std::expected<std::string, error_code>
    bytesToHexString(const types::Binary& bytes) noexcept
{
    try
    {
        std::ostringstream oss;
        oss << std::setfill('0') << std::hex;
        for (const auto& byte : bytes)
        {
            oss << std::setw(2) << static_cast<int>(byte);
        }
        return oss.str();
    }
    catch (const std::exception& ex)
    {
        lg2::error(
            "Error occured while converting to hex format, error {ERROR}",
            "ERROR", ex);
        return std::unexpected(error_code::STANDARD_EXCEPTION);
    }
}

/**
 * @brief Read the system IM keyword from D-Bus
 *
 * The API reads IM from dbus and convertes raw bytes into a printable hex
 * string.
 *
 * @return IM value as hex string, corresponding error code is returned on
 * failure.
 */
inline std::expected<std::string, error_code> getSystemIm() noexcept
{
    const auto res =
        readDbusProperty(constants::pimService, constants::systemInvPath,
                         constants::vsbpInterface, constants::kwdIM);

    if (!res)
    {
        return std::unexpected(res.error());
    }
    else if (const auto imBytes = std::get_if<types::Binary>(&res.value()))
    {
        return bytesToHexString(*imBytes);
    }

    return std::unexpected(error_code::RECEIVED_INVALID_KWD_TYPE_FROM_DBUS);
}

/**
 * @brief An API to create a PEL
 *
 * This API makes synchronous call to phosphor-logging Create method.
 *
 * @param[in] errIntf - Error Interface name
 * @param[in] severity -  Severity of the event
 * @param[in] additionalData - Additional information of PEL
 */
inline void createPEL(const std::string& errIntf, const std::string& severity,
                      const types::PelAdditionalData& additionalData) noexcept
{
    try
    {
        auto bus = sdbusplus::bus::new_default();
        auto method =
            bus.new_method_call(constants::eventLoggingServiceName,
                                constants::eventLoggingObjectPath,
                                constants::eventLoggingInterface, "Create");

        method.append(errIntf, severity, additionalData);
        bus.call(method);
    }
    catch (const sdbusplus::exception_t& ex)
    {
        lg2::error("PEL creation failed with an error: {ERROR}", "ERROR", ex);
    }
}
} // namespace utils
} // namespace panel
