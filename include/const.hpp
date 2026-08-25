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

static constexpr auto maxRetryCount = 6;

static constexpr auto deviceWriteFailure =
    "xyz.openbmc_project.Common.Device.Error.WriteFailure";
} // namespace constants
} // namespace panel
