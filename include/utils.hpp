#pragma once

#include "const.hpp"
#include "error_codes.hpp"
#include "types.hpp"

#include <expected>
#include <filesystem>
#include <fstream>
#include <nlohmann/json.hpp>
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

/**
 * @brief Build the panel configuration file path from an IM value.
 *
 * Constructs the path as:
 *   /usr/share/panel/panel_<im>.json
 *
 * @param[in] im - System IM value as a hex string (e.g. "70001000").
 *
 * @return Absolute path to the configuration file for that system.
 */
inline std::string getPanelConfigPath(const std::string& im) noexcept
{
    return std::string(constants::panelConfigBasePath) + "/panel_" + im +
           ".json";
}

/**
 * @brief API to parse respective JSON.
 *
 * @param[in] filePath - Path to JSON.
 *
 * @return Parsed JSON object on success; corresponding error code on
 * failure.
 */
inline std::expected<nlohmann::json, error_code>
    getParsedJson(const std::string& filePath) noexcept
{
    if (filePath.empty())
    {
        return std::unexpected(error_code::INVALID_INPUT_PARAMETER);
    }

    std::error_code ec;
    if (!std::filesystem::exists(filePath, ec))
    {
        return std::unexpected(ec ? error_code::FILE_ACCESS_ERROR
                                  : error_code::FILE_NOT_FOUND);
    }

    try
    {
        std::ifstream file(filePath);
        if (!file)
        {
            return std::unexpected(error_code::FILE_ACCESS_ERROR);
        }

        return nlohmann::json::parse(file);
    }
    catch (const nlohmann::json::parse_error& ex)
    {
        lg2::error("JSON parse error while parsing {PATH}: {ERROR}", "PATH",
                   filePath, "ERROR", ex.what());
        return std::unexpected(error_code::JSON_PARSE_ERROR);
    }
    catch (const std::exception& ex)
    {
        lg2::error("Unexpected error while parsing {PATH}: {ERROR}", "PATH",
                   filePath, "ERROR", ex.what());
        return std::unexpected(error_code::STANDARD_EXCEPTION);
    }
}

/**
 * @brief Validate "gpio" details in JSON object.
 *
 * API validates whether JSON contains required tags to process GPIO
 * information.
 *
 * @param[in] configJson - JSON object for gpio.
 *
 * @return Empty on success; corresponding error code on failure.
 */
inline std::expected<void, error_code>
    validateGpioTags([[maybe_unused]] const nlohmann::json& configJson) noexcept
{
    // ToDo: Validate gpio details in JSON
    return {};
}

/**
 * @brief Validate "lcdPanel" or "basePanel" tag in JSON configuration.
 *
 * @param[in] configJson - Parsed JSON object containing "lcdPanel" or
 * "basePanel" details.
 *
 * @return Empty on success; corresponding error code on failure.
 */
inline std::expected<void, error_code>
    validatePanelTags(const nlohmann::json& configJson) noexcept
{
    if (!configJson.is_object())
    {
        return std::unexpected(error_code::INVALID_JSON);
    }

    if (!configJson.contains("devicePath"))
    {
        return std::unexpected(error_code::DEVICE_PATH_NOT_FOUND);
    }
    else if (!configJson["devicePath"].is_string() ||
             configJson["devicePath"].get<std::string>().empty())
    {
        return std::unexpected(error_code::INVALID_DEVICE_PATH);
    }

    if (!configJson.contains("deviceAddress"))
    {
        return std::unexpected(error_code::DEVICE_ADDRESS_NOT_FOUND);
    }
    else if (!configJson["deviceAddress"].is_number_integer())
    {
        return std::unexpected(error_code::INVALID_DEVICE_ADDRESS);
    }

    if (!configJson.contains("objectPath"))
    {
        return std::unexpected(error_code::OBJECT_PATH_NOT_FOUND);
    }
    else if (!configJson["objectPath"].is_string() ||
             configJson["objectPath"].get<std::string>().empty())
    {
        return std::unexpected(error_code::INVALID_OBJECT_PATH);
    }

    if (configJson.contains("listenOnPanelPresence") &&
        !configJson["listenOnPanelPresence"].is_boolean())
    {
        return std::unexpected(error_code::INVALID_TAG_VALUE);
    }

    if (configJson.contains("requiresI2cEnable") &&
        !configJson["requiresI2cEnable"].is_boolean())
    {
        return std::unexpected(error_code::INVALID_TAG_VALUE);
    }

    if (configJson.contains("gpioPresence"))
    {
        if (const auto res = validateGpioTags(configJson["gpioPresence"]); !res)
        {
            return std::unexpected(res.error());
        }
    }

    return {};
}

/**
 * @brief Validate overall panel configuration JSON.
 *
 * @param[in] configJson - Parsed JSON object.
 *
 * @return Empty on success; corresponding error code on failure.
 */
inline std::expected<void, error_code>
    validateConfigJson(const nlohmann::json& configJson) noexcept
{
    if (!configJson.is_object() || configJson.empty())
    {
        return std::unexpected(error_code::INVALID_JSON);
    }

    if (!configJson.contains("lcdPanel"))
    {
        return std::unexpected(error_code::MISSING_LCD_PANEL_TAG);
    }

    if (const auto res = validatePanelTags(configJson["lcdPanel"]); !res)
    {
        return std::unexpected(res.error());
    }

    // ToDo: Validate Button input device path and listenOnProperties tag
    return {};
}
} // namespace utils
} // namespace panel
