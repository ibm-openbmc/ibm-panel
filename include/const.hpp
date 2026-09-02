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

static constexpr auto eventLoggingServiceName = "xyz.openbmc_project.Logging";
static constexpr auto eventLoggingObjectPath = "/xyz/openbmc_project/logging";
static constexpr auto eventLoggingInterface =
    "xyz.openbmc_project.Logging.Create";
} // namespace constants
} // namespace panel
