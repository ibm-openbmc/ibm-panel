#pragma once

#include <cstdint>
#include <map>
#include <string>
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

using PelAdditionalData = std::map<std::string, std::string>;
} // namespace types
} // namespace panel