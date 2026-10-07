#pragma once

#include <string>
#include <unordered_map>

enum error_code
{
    // Dbus errors
    DBUS_FAILURE,
    INVALID_VALUE_READ_FROM_DBUS,
    RECEIVED_INVALID_KWD_TYPE_FROM_DBUS,

    // Generic errors
    INVALID_INPUT_PARAMETER,
    STANDARD_EXCEPTION,
    DEVICE_PATH_NOT_FOUND,
    INVALID_DEVICE_PATH,
    DEVICE_ADDRESS_NOT_FOUND,
    INVALID_DEVICE_ADDRESS,
    OBJECT_PATH_NOT_FOUND,
    INVALID_OBJECT_PATH,
    GPIO_PIN_NOT_FOUND,
    INVALID_GPIO_PIN,
    GPIO_VALUE_NOT_FOUND,
    INVALID_GPIO_VALUE,
    INVALID_SERVICE_NAME,
    INVALID_INTERFACE_NAME,
    INVALID_PROPERTY_NAME,

    // File exceptions
    FILE_NOT_FOUND,
    FILE_ACCESS_ERROR,

    // JSON exceptions
    JSON_PARSE_ERROR,
    INVALID_JSON,
    INVALID_TAG_VALUE,
    MISSING_LCD_PANEL_TAG,
    MISSING_LISTEN_ON_PROPERTIES_TAG,
    MISSING_INTERFACE_TAG,
    MISSING_PROPERTIES_TAG,
};

// Error code to error message map
const std::unordered_map<int, std::string> errorCodeMap = {
    {error_code::DBUS_FAILURE, "Dbus call failed"},
    {error_code::INVALID_VALUE_READ_FROM_DBUS, "Invalid value read from DBus"},
    {error_code::RECEIVED_INVALID_KWD_TYPE_FROM_DBUS,
     "Received invalid keyword data type from DBus."},
    {error_code::INVALID_INPUT_PARAMETER,
     "Either one of the input parameter is invalid or empty."},
    {error_code::STANDARD_EXCEPTION, "Standard Exception thrown"},
    {error_code::FILE_NOT_FOUND, "File does not exist."},
    {error_code::FILE_ACCESS_ERROR, "Failed to access the file."},
    {error_code::JSON_PARSE_ERROR, "Error while parsing JSON file."},
    {error_code::INVALID_JSON, "Invalid JSON object."},
    {error_code::DEVICE_PATH_NOT_FOUND, "The device path not found."},
    {error_code::INVALID_DEVICE_PATH, "The device path is invalid or empty."},
    {error_code::DEVICE_ADDRESS_NOT_FOUND, "The device address not found."},
    {error_code::INVALID_DEVICE_ADDRESS, "The device address is invalid."},
    {error_code::OBJECT_PATH_NOT_FOUND, "The object path not found."},
    {error_code::INVALID_OBJECT_PATH, "The object path is invalid or empty."},
    {error_code::INVALID_TAG_VALUE,
     "The tag value or type in the JSON is invalid."},
    {error_code::MISSING_LCD_PANEL_TAG, "Required lcdPanel tag is missing."},
    {error_code::GPIO_PIN_NOT_FOUND, "The GPIO pin is not found."},
    {error_code::INVALID_GPIO_PIN, "The GPIO pin is invalid or empty."},
    {error_code::GPIO_VALUE_NOT_FOUND, "The GPIO value is not found."},
    {error_code::INVALID_GPIO_VALUE, "The GPIO value is invalid."},
    {error_code::INVALID_SERVICE_NAME, "The service name is invalid or empty."},
    {error_code::INVALID_INTERFACE_NAME,
     "The interface name is invalid or empty."},
    {error_code::MISSING_INTERFACE_TAG,
     "Required interface tag is missing in the JSON."},
    {error_code::MISSING_PROPERTIES_TAG,
     "Required properties tag is missing in the JSON."},
    {error_code::INVALID_PROPERTY_NAME,
     "The property name is invalid or empty."},
    {MISSING_LISTEN_ON_PROPERTIES_TAG,
     "Required listenOnProperties tag is missing in the JSON"},
};