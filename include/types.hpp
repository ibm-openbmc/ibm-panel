#pragma once

#include <cstdint>
#include <map>
#include <string>
#include <tuple>
#include <variant>
#include <vector>

namespace panel
{
namespace types
{

using Byte = uint8_t;
using Binary = std::vector<Byte>;

using FunctionNumber = uint8_t;
using FunctionMask = uint16_t;
using FunctionalityList = std::vector<FunctionNumber>;
using RoleType = uint16_t;
using systemStateType = uint16_t;

// This covers mostly all the data type supported over Dbus for a property.
// clang-format off
using DbusVariantType = std::variant<
    std::vector<std::tuple<std::string, std::string, std::string>>,
    types::Binary>;

// Pel additional data contains map of key and value
using PelAdditionalData = std::map<std::string, std::string>;
} // namespace types
} // namespace panel