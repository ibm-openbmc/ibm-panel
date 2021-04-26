#pragma once

#include <iostream>
#include <vector>

namespace panel
{
namespace types
{

using FunctionNumber = uint8_t;
using FunctionalityList = std::vector<FunctionNumber>;
using Byte = uint8_t;
using Binary = std::vector<Byte>;

enum ButtonEvent
{
    INCREMENT,
    DECREMENT,
    EXECUTE
};

} // namespace types
} // namespace panel