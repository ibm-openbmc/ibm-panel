#pragma once

#include "types.hpp"

namespace panel
{
namespace constants
{

static constexpr auto baseDevPath = "/dev/i2c-3";
static constexpr auto rainLcdDevPath = "/dev/i2c-7";
static constexpr auto everLcdDevPath = "/dev/i2c-28";
static constexpr auto tacomaLcdDevPath = "/dev/i2c-0";

static constexpr auto devAddr = 0x5a;

static constexpr auto systemDbusObj =
    "/xyz/openbmc_project/inventory/system/chassis/motherboard";
static constexpr auto rainBaseDbusObj =
    "/xyz/openbmc_project/inventory/system/chassis/motherboard/"
    "base_op_panel_blyth";
static constexpr auto everBaseDbusObj =
    "/xyz/openbmc_project/inventory/system/chassis/motherboard/dasd_backplane/"
    "panel0";
static constexpr auto rainLcdDbusObj = "/xyz/openbmc_project/inventory/system/"
                                       "chassis/motherboard/lcd_op_panel_hill";
static constexpr auto everLcdDbusObj =
    "/xyz/openbmc_project/inventory/system/chassis/motherboard/dasd_backplane/"
    "panel1";

static constexpr auto rain2s2uIM = "50001001";
static constexpr auto rain2s4uIM = "50001000";
static constexpr auto rain1s4uIM = "50001002";
static constexpr auto everestIM = "50003000";

static constexpr auto imInterface = "com.ibm.ipzvpd.VSBP";
static constexpr auto imKeyword = "IM";
static constexpr auto itemInterface = "xyz.openbmc_project.Inventory.Item";
static constexpr auto inventoryManagerIntf =
    "xyz.openbmc_project.Inventory.Manager";
static constexpr auto networkManagerService = "xyz.openbmc_project.Network";
static constexpr auto networkManagerObj = "/xyz/openbmc_project/network";
static constexpr auto locCodeIntf =
    "xyz.openbmc_project.Inventory.Decorator.LocationCode";

static constexpr auto tmKwdDataLength = 8;
static constexpr auto ccinDataLength = 4;
static constexpr auto fiveHexWordsWithSpaces = 44;
static constexpr auto terminatingBit = 2;

// Progress code src equivalent to  ascii "00000000"
static constexpr auto clearDisplayProgressCode = 0x3030303030303030;
static constexpr auto loggerObjectPath = "/xyz/openbmc_project/logging";
static constexpr auto loggerCreateInterface =
    "xyz.openbmc_project.Logging.Create";
static constexpr auto mapperObjectPath = "/xyz/openbmc_project/object_mapper";
static constexpr auto mapperInterface = "xyz.openbmc_project.ObjectMapper";
static constexpr auto mapperDestination = "xyz.openbmc_project.ObjectMapper";

// Minimum version for both base and LCD panels
static const types::PanelVersion minPanelVersion('5', '0');
static const types::PanelVersion maxLCDVersion('5', '2');
static const types::PanelVersion maxBaseVersion('5', '5');
static constexpr auto maxFlashWriteChunk = 68;
} // namespace constants
} // namespace panel