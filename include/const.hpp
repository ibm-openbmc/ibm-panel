#pragma once

#include "types.hpp"

#include <cstdint>

namespace panel
{
namespace constants
{

static constexpr auto panelService = "com.ibm.PanelApp";
static constexpr auto panelInterface = "com.ibm.panel";
static constexpr auto panelObjectPath = "/com/ibm/panel_app";

static constexpr auto redundancyService =
    "xyz.openbmc_project.State.BMC.Redundancy";
static constexpr auto redundancyObjectPath = "/xyz/openbmc_project/state/bmc0";
static constexpr auto redundancyInterface =
    "xyz.openbmc_project.State.BMC.Redundancy";
static constexpr auto roleProperty = "Role";
static constexpr auto rolePassive =
    "xyz.openbmc_project.State.BMC.Redundancy.Role.Passive";
static constexpr auto roleActive =
    "xyz.openbmc_project.State.BMC.Redundancy.Role.Active";
static constexpr auto roleUnknown =
    "xyz.openbmc_project.State.BMC.Redundancy.Role.Unknown";
} // namespace constants
} // namespace panel
