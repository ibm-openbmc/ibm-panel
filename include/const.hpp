#pragma once

#include "types.hpp"

#include <array>
#include <cstdint>
#include <string_view>

namespace panel
{
namespace constants
{

static constexpr auto panelService = "com.ibm.PanelApp";
static constexpr auto panelInterface = "com.ibm.panel";
static constexpr auto panelObjectPath = "/com/ibm/panel_app";

static constexpr std::string_view huygensIm = "70001000";

// List of redundant-BMC system IM values
static constexpr std::array<std::string_view, 1> redundantBmcSystemImList{
    huygensIm,
};

// Default BMC role considered at application startup for redundant BMC systems.
static constexpr auto roleUnknown = 0x40;

// Represents all BMC role bits (Active + Passive + Unknown),
// allowing the role to be ignored for single BMC systems.
static constexpr auto roleMask = 0x1C0;

} // namespace constants
} // namespace panel
