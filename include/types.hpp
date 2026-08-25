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

// Key-value map used to pass additional callout data when creating a PEL
using PelAdditionalData = std::map<std::string, std::string>;

} // namespace types
} // namespace panel