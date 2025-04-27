#pragma once

#include "types.hpp"

#include <cstdint>

namespace panel
{
namespace constants
{

static constexpr auto baseDevPath = "/dev/i2c-3";
static constexpr auto balconesBaseDevPath = "/dev/i2c-2";
static constexpr auto rainLcdDevPath = "/dev/i2c-7";
static constexpr auto everLcdDevPath = "/dev/i2c-28";
static constexpr auto tacomaLcdDevPath = "/dev/i2c-0";
static constexpr auto blueridgeLcdDevPath = "/dev/i2c-7";
static constexpr auto fujiLcdDevPath = "/dev/i2c-28";
static constexpr auto balconesLcdDevPath = rainLcdDevPath;

static constexpr auto devAddr = 0x5a;

static constexpr auto systemDbusObj =
    "/xyz/openbmc_project/inventory/system/chassis/motherboard";
static constexpr auto rainBaseDbusObj =
    "/xyz/openbmc_project/inventory/system/chassis/motherboard/"
    "base_op_panel_blyth";
static constexpr auto everBaseDbusObj =
    "/xyz/openbmc_project/inventory/system/chassis/motherboard/dasd_backplane/"
    "panel0";
static constexpr auto balconesBaseDbusObj =
    "/xyz/openbmc_project/inventory/system/chassis/motherboard/base_op_panel";
static constexpr auto blueridgeBaseDbusObj = rainBaseDbusObj;
static constexpr auto fujiBaseDbusObj = everBaseDbusObj;

static constexpr auto rainLcdDbusObj = "/xyz/openbmc_project/inventory/system/"
                                       "chassis/motherboard/lcd_op_panel_hill";
static constexpr auto everLcdDbusObj =
    "/xyz/openbmc_project/inventory/system/chassis/motherboard/dasd_backplane/"
    "panel1";
static constexpr auto blueridgeLcdDbusObj = rainLcdDbusObj;
static constexpr auto fujiLcdDbusObj = everLcdDbusObj;

static constexpr auto everBMCObj =
    "/xyz/openbmc_project/inventory/system/chassis/motherboard/bmc";
static constexpr auto balconesLcdDbusObj =
    "/xyz/openbmc_project/inventory/system/chassis/motherboard/lcd_op_panel";

static constexpr auto rain2s2uIM = "50001001";
static constexpr auto rain2s4uIM = "50001000";
static constexpr auto rain1s4uIM = "50001002";
static constexpr auto everestIM = "50003000";
static constexpr auto balconesIM = "60004000";
static constexpr auto blueridge2s2uIM = "60001001";
static constexpr auto blueridge2s4uIM = "60001000";
static constexpr auto blueridge1s4uIM = "60001002";
static constexpr auto fujiIM = "60002000";

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
static constexpr auto terminatingBits = 0xA; // Checkstop and Term due to FW

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

static constexpr auto deviceReadFailure =
    "xyz.openbmc_project.Common.Device.Error.ReadFailure";
static constexpr auto deviceWriteFailure =
    "xyz.openbmc_project.Common.Device.Error.WriteFailure";
static constexpr auto codeUpdateFailure =
    "com.ibm.Panel.Error.CodeUpdateFailure";

// Errno defines
static constexpr auto errnoNoDeviceOrAddress = 6;

// Map of system type to that of path to the FRU on which BootFail PIC is
// physically present
static const types::PICFRUPathMap bootFailPIC = {
    {rain2s2uIM, systemDbusObj},
    {rain2s4uIM, systemDbusObj},
    {rain1s4uIM, systemDbusObj},
    {everestIM, everBMCObj},
    {balconesIM, systemDbusObj},
    {blueridge2s2uIM, systemDbusObj},
    {blueridge2s4uIM, systemDbusObj},
    {blueridge1s4uIM, systemDbusObj},
    {fujiIM, everBMCObj}};

static const uint8_t FUNCTION_12 = 12;
static const uint8_t FUNCTION_13 = 13;

/** PEL SRC data structure has the following sub sections:
 * 1. 8 byte header.
 * 2. HEX words each of length 4 bytes.
 * 3. 32 bytes ascii character string.
 *
 * Refer PEL SRC data structure for more details.
 */
static const uint8_t FIRST_HEX_WORD_START_OFFSET = 8;
static const uint8_t LAST_HEX_WORD_START_OFFSET = 36;
static const uint8_t LEN_OF_RAW_HEX_WORD = 4;
static const uint8_t WORDS_PER_DISPLAY = 4;

static const std::string defaultHexWordValue = "00000000";

// Maximum valid instance IDs' which can be created by libpldm
static constexpr auto PLDM_INST_ID_MAX = 32;
} // namespace constants
} // namespace panel
