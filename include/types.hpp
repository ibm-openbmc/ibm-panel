#pragma once

#include <iostream>
#include <map>
#include <tuple>
#include <unordered_map>
#include <variant>
#include <vector>

namespace panel
{
namespace types
{

using FunctionNumber = uint8_t;
using FunctionalityList = std::vector<FunctionNumber>;
using Byte = uint8_t;
using Binary = std::vector<Byte>;
using PanelDataTuple = std::tuple<std::string, uint8_t, std::string>;
using PanelDataMap = std::unordered_map<std::string, PanelDataTuple>;
using ItemInterfaceMap = std::map<std::string, std::variant<bool, std::string>>;

enum ButtonEvent
{
    INCREMENT,
    DECREMENT,
    EXECUTE
};

} // namespace types
} // namespace panel