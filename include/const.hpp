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

// list of High-end system IM values
static constexpr std::array<std::string_view, 1> highEndSystemImList{
    huygensIm,
};

// Constants related to BMC role
static constexpr auto roleUnknown = 0x40;
static constexpr auto roleMask =
    0x1C0; // all role bits are high (Active+Passive+Unknow)

} // namespace constants
} // namespace panel
