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

    // File exceptions
    FILE_NOT_FOUND,
    FILE_ACCESS_ERROR,

    // JSON exceptions
    JSON_PARSE_ERROR,
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
};