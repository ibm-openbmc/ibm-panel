#pragma once

#include <iostream>
#include <vector>

namespace panel
{
namespace types
{

using FunctionNumber = uint8_t;
using FunctionalityList = std::vector<FunctionNumber>;

enum ButtonEvent
{
    INCREMENT,
    DECREMENT,
    EXECUTE
};

} // namespace types
} // namespace panel