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
static constexpr auto phypTerminusID = (types::Byte)208;

// Reference: PLDM Entity and State Set IDs in DSP0249_1.1.0 specification.
// The Virtual machine manager is a logical entity, so bit 15 need to be set.
// VMM Entity ID is referred from
// https://github.ibm.com/openbmc/pldm/blob/1020/libpldm/entity.h#L34
static constexpr uint16_t vmmEntityId = 33 | 0x8000; // logical value - 32801
// TODO: Add the reference to the state set id present in pldm state_set.h
// header file. Currently this info is not in pldm_state_set.h.
static constexpr uint16_t osIplModeStateSetId = (uint16_t)32777;

} // namespace constants
} // namespace panel