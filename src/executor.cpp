#include "executor.hpp"

#include "const.hpp"
#include "exception.hpp"
#include "pldm_fw.hpp"
#include "utils.hpp"

#include <boost/algorithm/string.hpp>
#include <boost/asio/steady_timer.hpp>
#include <string_view>

namespace panel
{
void Executor::displayExecutionStatus(
    const types::FunctionNumber funcNumber,
    const types::FunctionalityList& subFuncNumber, const bool result)
{
    std::ostringstream convert;
    convert << std::setfill('0') << std::setw(2)
            << static_cast<int>(funcNumber);

    if (!subFuncNumber.empty())
    {
        convert << std::setfill('0') << std::setw(2)
                << static_cast<int>(subFuncNumber.at(0));
    }
    else
    {
        convert << "   ";
    }

    convert << (result ? " 00" : " FF");
    utils::sendCurrDisplayToPanel(convert.str(), "", transport);
}

void Executor::executeFunction(const types::FunctionNumber funcNumber,
                               const types::FunctionalityList& subFuncNumber)
{
    // test output, to be removed
    std::cout << funcNumber << std::endl;

    if (!subFuncNumber.empty())
    {
        std::cout << subFuncNumber.at(0) << std::endl;
    }

    // If function 25 has been executed last and the current requested function
    // for execution is not 25 or 26 then reset the states else maintain
    // function 25 executed state.
    // Function 26 is included here as func26 execution will need the state of
    // func25.
    if (serviceSwitch1State && funcNumber != 25 && funcNumber != 26)
    {
        serviceSwitch1State = false;
    }

    try
    {
        switch (funcNumber)
        {
            case 1:
                execute01();
                break;

            case 2:
                execute02(subFuncNumber);
                break;

            case 3:
                execute03();
                break;

            case 4:
                execute04();
                break;

            case 8:
                execute08();
                break;

            case 11:
                execute11();
                break;

            case 12:
                execute12();
                break;

            case 13:
                execute13();
                break;

            case 14:
            case 15:
            case 16:
            case 17:
            case 18:
            case 19:
                execute14to19(funcNumber);
                break;
            case 20:
                execute20();
                break;

            case 21:
            case 22:
            case 34:
            case 41:
            case 65:
            case 66:
            case 67:
            case 68:
            case 69:
            case 70:
                sendFuncNumToPhyp(funcNumber);
                break;

            case 25:
                execute25();
                break;

            case 26:
                execute26();
                break;

            case 30:
                execute30(subFuncNumber);
                break;

            case 42:
                execute42();
                break;

            case 43:
                execute43();
                break;

            case 55:
                execute55(subFuncNumber);
                break;

            case 63:
                execute63(subFuncNumber.at(0));
                break;

            case 64:
                execute64(subFuncNumber.at(0));
                break;

            case 73:
                execute73();
                break;

            case 74:
                execute74();
                break;

            default:
                break;
        }
    }
    catch (BaseException& e)
    {
        std::cerr << e.what() << std::endl;
        displayExecutionStatus(funcNumber, subFuncNumber, false);

        // In case of function 02, if there is any exception, throw it
        // back to state manager so that setting operating modes can be avoided.
        if (funcNumber == 2)
        {
            throw;
        }
    }
    catch (const sdbusplus::exception::SdBusError& e)
    {
        std::cerr << e.what() << std::endl;
        displayExecutionStatus(funcNumber, subFuncNumber, false);

        // In case of function 02, if there is any Dbus read/write error, throw
        // it back to state manager so that setting operating modes can be
        // avoided.
        if (funcNumber == 2)
        {
            throw;
        }
    }
    catch (const boost::system::system_error& err)
    {
        std::cerr << "Boost throwing exception" << err.what() << std::endl;
        displayExecutionStatus(funcNumber, subFuncNumber, false);
    }
}

void Executor::execute03()
{
    utils::writeBusProperty<std::string>(
        "xyz.openbmc_project.State.Host", "/xyz/openbmc_project/state/host0",
        "xyz.openbmc_project.State.Host", "RequestedHostTransition",
        "xyz.openbmc_project.State.Host.Transition.GracefulWarmReboot");

    utils::sendCurrDisplayToPanel("RESTART SERVER", "INITIATED", transport);
}

void Executor::execute25()
{
    // Execution will not reach this point for subsequent function 25 request
    // once the state is set.
    serviceSwitch1State = !serviceSwitch1State;
    if (serviceSwitch1State)
    {
        // Display 00 only when function 25 state is set.
        displayExecutionStatus(25, std::vector<types::FunctionNumber>(), true);
    }
}

void Executor::execute26()
{
    if (serviceSwitch1State)
    {
        // function 26 has been successfully executed, reset function 25 state.
        serviceSwitch1State = false;
        displayExecutionStatus(26, std::vector<types::FunctionNumber>(), true);
    }
    else
    {
        throw FunctionFailure(
            "Function 25 is not executed prior to function 26");
    }
}

void Executor::execute20()
{
    auto res = utils::readBusProperty<std::variant<std::string>>(
        "xyz.openbmc_project.Inventory.Manager",
        "/xyz/openbmc_project/inventory/system",
        "xyz.openbmc_project.Inventory.Decorator.Asset", "SerialNumber");

    std::string line1(16, ' ');
    std::string line2(16, ' ');

    const auto serialNumber = std::get_if<std::string>(&res);
    if (serialNumber != nullptr)
    {
        line2.replace(0, (*serialNumber).length(), *serialNumber);
    }

    // reading machine model type
    auto resValue = utils::readBusProperty<std::variant<types::Binary>>(
        "xyz.openbmc_project.Inventory.Manager",
        "/xyz/openbmc_project/inventory/system/chassis/motherboard",
        "com.ibm.ipzvpd.VSYS", "TM");

    const auto propData = std::get_if<types::Binary>(&resValue);

    if (propData != nullptr)
    {
        line1.replace(0, constants::tmKwdDataLength,
                      reinterpret_cast<const char*>(propData->data()));
    }

    // reading CCIN
    res = utils::readBusProperty<std::variant<std::string>>(
        "xyz.openbmc_project.Inventory.Manager",
        "/xyz/openbmc_project/inventory/system/chassis/motherboard",
        "xyz.openbmc_project.Inventory.Decorator.Asset", "Model");

    const auto model = std::get_if<std::string>(&res);
    if (model != nullptr)
    {
        line1.replace(11, constants::ccinDataLength, *model);
    }

    if ((line1.compare(std::string(16, ' ')) == 0) &&
        (line2.compare(std::string(16, ' ')) == 0))
    {
        throw FunctionFailure("Function 20 failed.");
    }
    utils::sendCurrDisplayToPanel(line1, line2, transport);
}

void Executor::execute11()
{
    auto srcData = pelEventIdQueue.back();

    if (!srcData.empty())
    {
        // find the first space to get response code
        auto pos = srcData.find_first_of(" ");

        // length of src data need to be 8
        if (pos != std::string::npos && pos == 8)
        {
            utils::sendCurrDisplayToPanel((srcData).substr(0, pos),
                                          std::string{}, transport);
        }
        else
        {
            std::cerr << "Invalid srcData received = " << srcData << std::endl;
        }
        return;
    }

    // TODO: Decide what needs to be done in this case.
    std::cerr << "Error getting SRC data" << std::endl;
}

static std::string getEthObjByIntf(const std::string& portName)
{
    std::string invEthObj = "/xyz/openbmc_project/inventory/system/chassis/"
                            "motherboard/ebmc_card_bmc/ethernet0";

    if (portName == "eth1")
    {
        invEthObj = "/xyz/openbmc_project/inventory/system/chassis/motherboard/"
                    "ebmc_card_bmc/ethernet1";
    }
    return invEthObj;
}

static std::string getEthernetMac(const std::string& portName)
{
    std::string invEthObj = getEthObjByIntf(portName);
    const auto nwItemIntf =
        "xyz.openbmc_project.Inventory.Item.NetworkInterface";
    auto ethMac = utils::readBusProperty<std::variant<std::string>>(
        constants::inventoryManagerIntf, invEthObj, nwItemIntf, "MACAddress");
    if (auto p = std::get_if<std::string>(&ethMac))
    {
        return *p;
    }
    return {};
}

static std::string getEthernetLocation(const std::string& portName)
{
    std::string invEthObj = getEthObjByIntf(portName);
    auto locCode = utils::readBusProperty<std::variant<std::string>>(
        constants::inventoryManagerIntf, invEthObj, constants::locCodeIntf,
        "LocationCode");
    if (auto p = std::get_if<std::string>(&locCode))
    {
        return *p;
    }
    return {};
}

static std::string getPortSegment(const std::string& locCode)
{
    // U78DB.ND0.WZS0008-P0-C5-T0
    auto pos = locCode.find_last_of('-');
    std::string loc = locCode.substr(pos + 1);
    return loc;
}

void Executor::execute30(const types::FunctionalityList& subFuncNumber)
{
    // call Get Managed Objects for Network manager
    const auto& networkObjects = utils::getManagedObjects(
        constants::networkManagerService, constants::networkManagerObj);

    std::string ethPort = "eth0";
    std::string otherPort = "eth1";
    if (subFuncNumber.at(0) == 0x01) // eth1
    {
        ethPort = "eth1";
        otherPort = "eth0";
    }

    // If address points to invalid value, default 0.0.0.0 will be displayed.
    std::string line2 = "0.0.0.0";
    std::string ethObjPath = constants::networkManagerObj;
    ethObjPath += "/";
    ethObjPath += ethPort;
    std::string macAddr{}, locCode{};

    for (const auto& obj : networkObjects)
    {
        std::string originStr{};
        const std::string& objPath =
            std::get<sdbusplus::message::object_path>(obj);
        // search for eth0/eth1 ipv4 objects based on input
        // eg: /xyz/openbmc_project/network/eth0/ipv4/bb39e832
        std::string ethV4 = "/";
        ethV4 += ethPort;
        ethV4 += "/ipv4/";
        const auto& intfPropVector = std::get<1>(obj);
        if (objPath.find(ethV4) != std::string::npos)
        {
            for (const auto& intfProp : intfPropVector)
            {
                const auto& ipInterface = "xyz.openbmc_project.Network.IP";
                if (std::get<0>(intfProp) == ipInterface)
                {
                    const auto& propValMap = std::get<1>(intfProp);
                    const auto& originItr = propValMap.find("Origin");
                    const auto& typeItr = propValMap.find("Type");

                    if ((typeItr != propValMap.end()) &&
                        (originItr != propValMap.end()))
                    {
                        const auto& type =
                            std::get_if<std::string>(&(typeItr->second));
                        const auto& origin =
                            std::get_if<std::string>(&(originItr->second));

                        if ((type != nullptr) && (origin != nullptr) &&
                            (*type == "xyz.openbmc_project.Network.IP."
                                      "Protocol.IPv4"))
                        {
                            originStr = *origin;
                            const auto& addressItr = propValMap.find("Address");
                            if (addressItr != propValMap.end())
                            {
                                const auto& address = std::get_if<std::string>(
                                    &(addressItr->second));
                                if (address != nullptr)
                                {
                                    line2 = *address;
                                }
                            }
                        }
                    }
                    break;
                }
            }
        }
        if (objPath == ethObjPath)
        {
            auto intfItr = std::find_if(
                intfPropVector.begin(), intfPropVector.end(),
                [](const auto& intf) {
                    return (intf.first ==
                            "xyz.openbmc_project.Network.MACAddress");
                });
            if (intfItr == intfPropVector.end())
            {
                std::cerr << "Mac address interface not found." << std::endl;
            }
            const auto& macAddrItr = intfItr->second.find("MACAddress");
            if (macAddrItr == intfItr->second.end())
            {
                std::cerr << "MACAddress property not found." << std::endl;
            }
            if (auto mac = std::get_if<std::string>(&(macAddrItr->second)))
            {
                macAddr = *mac;
            }
        }

        // If MAC address and static/dynamic IP are found, break the loop. Else
        // LinkLocal IP will be displayed by default.
        if (!macAddr.empty() &&
            (line2 != "0.0.0.0" &&
             ((originStr ==
               "xyz.openbmc_project.Network.IP.AddressOrigin.Static") ||
              (originStr ==
               "xyz.openbmc_project.Network.IP.AddressOrigin.DHCP"))))
        {
            break;
        }
    }

    // obtain the mac address of network ethernet object path. query mac
    // address of ethernet0&1 of inv manager objects. if mac of both the
    // objects(network & inventory eth objects)matches, take loc code from
    // the respective inv manager obj path.

    if (macAddr == getEthernetMac(ethPort))
    {
        locCode = getEthernetLocation(ethPort);
    }
    else if (macAddr == getEthernetMac(otherPort))
    {
        locCode = getEthernetLocation(otherPort);
    }
    else
    {
        std::cerr << "\n No matching ethernet object in Inventory Manager. "
                  << std::endl;
    }

    if (!locCode.empty())
    {
        locCode = getPortSegment(locCode);
    }

    if ((locCode.empty() && line2.empty()) || line2.empty())
    {
        throw FunctionFailure("Function 30 failed.");
    }

    // create display
    std::string line1 = "SP: ";
    line1 += boost::to_upper_copy<std::string>(ethPort);
    line1 += ":      ";
    line1 += locCode;
    panel::utils::sendCurrDisplayToPanel(line1, line2, transport);
}

bool Executor::isOSIPLTypeEnabled() const
{
    // TODO: Check with PLDM how they will communicate if IPL type is
    // enabled or disabled. Till then return dummy value as false.
    return false;
}

void Executor::execute01()
{
    const auto sysValues = utils::readSystemParameters();

    std::string line1(16, ' ');
    std::string line2(16, ' ');

    if (isOSIPLTypeEnabled())
    {
        // OS IPL Type
        line1.replace(4, 1, std::get<0>(sysValues).substr(0, 1));
    }

    // Operating mode
    line1.replace(7, 1, std::get<1>(sysValues).substr(0, 1));

    // hypervisor type
    if (std::get<4>(sysValues) == "PowerVM")
    {
        line1.replace(12, 3, "PVM");
    }
    else
    {
        line1.replace(12, std::get<4>(sysValues).length(),
                      std::get<4>(sysValues));
    }

    // HMC Managed
    if (std::get<2>(sysValues) == "1")
    {
        line2.replace(0, 5, "HMC=1");
    }

    // read the next boot side selected
    std::string nextBootSide{};
    utils::getNextBootSide(nextBootSide);

    // Add boot side to display.
    line2.replace(12, 1, nextBootSide);

    if ((line1.compare(std::string(16, ' ')) == 0) &&
        (line2.compare(std::string(16, ' ')) == 0))
    {
        throw FunctionFailure("Function 01 failed.");
    }
    // function number
    line1.replace(0, 2, "01");

    utils::sendCurrDisplayToPanel(line1, line2, transport);
    return;
}

void Executor::execute12()
{
    auto srcData = pelEventIdQueue.back();

    // Need to show blank spaces in case no srcData as function is enabled.
    constexpr auto blankHexWord = "        ";
    std::vector<std::string> output(4, blankHexWord);

    if (!srcData.empty())
    {
        std::vector<std::string> src;
        boost::split(src, srcData, boost::is_any_of(" "));

        const auto size = std::min(src.size(), (size_t)5);
        // ignoring the first hexword
        for (size_t i = 1; i < size; ++i)
        {
            output[i - 1] = src[i];
        }
    }

    // send blank display if string is empty
    utils::sendCurrDisplayToPanel((output.at(0) + output.at(1)),
                                  (output.at(2) + output.at(3)), transport);
}

void Executor::execute13()
{
    auto srcData = pelEventIdQueue.back();

    // Need to show blank spaces in case of no srcData as function is
    // enabled.
    constexpr auto blankHexWord = "        ";
    std::vector<std::string> output(4, blankHexWord);

    if (!srcData.empty())
    {
        std::vector<std::string> src;
        boost::split(src, srcData, boost::is_any_of(" "));

        const auto size = std::min(src.size(), (size_t)9);
        // ignoring the first five hexword
        for (size_t i = 5; i < size; ++i)
        {
            output[i - 5] = src[i];
        }
    }

    // send blank display if string is empty
    utils::sendCurrDisplayToPanel((output.at(0) + output.at(1)),
                                  (output.at(2) + output.at(3)), transport);
}

void Executor::execute14to19(const types::FunctionNumber funcNumber)
{
    std::string aCallOut{};
    std::string line1(16, ' ');
    std::string line2(16, ' ');

    switch (funcNumber)
    {
        // size check is not done here as functions are enabled based on
        // count of entries in this vector.
        case 14:
            aCallOut = callOutList.at(0);
            break;

        case 15:
            aCallOut = callOutList.at(1);
            break;

        case 16:
            aCallOut = callOutList.at(2);
            break;

        case 17:
            aCallOut = callOutList.at(3);
            break;

        case 18:
            aCallOut = callOutList.at(4);
            break;

        case 19:
            aCallOut = callOutList.at(5);
            break;
    }

    // sample resolution string in case of procedure callout
    // 1. Priority: High, Procedure: BMCSP02
    // output:
    // L1 : H -BMCSP02
    // L2 :

    // sample resolution string in hardware callout
    // 1. Location Code: U78DA.ND0.1234567-P0, Priority: Medium, PN: SVCDOCS
    // output:
    // L1 : M -SVCDOCS
    // L2 : U78DA.ND0.1234567-P0

    std::vector<std::string> callOutPropertyList;
    boost::split(callOutPropertyList, aCallOut, boost::is_any_of(","));

    for (auto& aProperty : callOutPropertyList)
    {
        std::vector<std::string> propValueList;
        boost::split(propValueList, aProperty, boost::is_any_of(":"));

        size_t pos = std::string::npos;
        if ((pos = propValueList[0].find("Priority")) != std::string::npos)
        {
            line1.replace(0, 1, propValueList[1].substr(1, 1));
        }
        else if ((pos = propValueList[0].find("Procedure")) !=
                 std::string::npos)
        {
            line1.replace(2, 1, "-");
            line1.replace(3, 7, propValueList[1].substr(1));
        }
        else if ((pos = propValueList[0].find("Location Code")) !=
                 std::string::npos)
        {
            line2.replace(0, (propValueList[1].substr(1)).length(),
                          propValueList[1].substr(1));
        }
        else if ((pos = propValueList[0].find("PN")) != std::string::npos)
        {
            line1.replace(2, 1, "-");
            line1.replace(3, 7, propValueList[1].substr(1));
        }
        // TODO: Currently, CCIN is not in data recieved from D-Bus.
    }

    if ((line1.compare(std::string(16, ' ')) == 0) &&
        (line2.compare(std::string(16, ' ')) == 0))
    {
        throw FunctionFailure(
            "Failed parsing resolution string during callout.");
    }
    utils::sendCurrDisplayToPanel(line1, line2, transport);
}
static std::string getIplType(const uint8_t index)
{
    switch (index)
    {
        case 0:
            return "A_Mode";
        case 1:
            return "B_Mode";
        case 2:
            return "C_Mode";
        default:
            return "D_Mode";
    }
}

static types::PendingAttributesItemType
    setOperatingMode(const uint8_t sysOperatingModeIndex)
{
    // Normal mode is the default mode hence all the defaul values are as
    // per normal mode.
    types::AttributeValueType sysOperatingModeValue = "Normal";
    std::string PowerRestorePolicy =
        "xyz.openbmc_project.Control.Power.RestorePolicy.Policy.Restore";
    bool autoReboot = true;

    if (sysOperatingModeIndex == 0)
    {
        sysOperatingModeValue = "Manual";
        PowerRestorePolicy = "xyz.openbmc_project.Control.Power."
                             "RestorePolicy.Policy.AlwaysOff";
        autoReboot = false;
    }

    utils::writeBusProperty<std::string>(
        "xyz.openbmc_project.Settings",
        "/xyz/openbmc_project/control/host0/power_restore_policy",
        "xyz.openbmc_project.Control.Power.RestorePolicy", "PowerRestorePolicy",
        PowerRestorePolicy);

    utils::writeBusProperty<bool>(
        "xyz.openbmc_project.Settings",
        "/xyz/openbmc_project/control/host0/auto_reboot",
        "xyz.openbmc_project.Control.Boot.RebootPolicy", "AutoReboot",
        autoReboot);

    return std::make_pair(
        "pvm_system_operating_mode",
        std::make_tuple("xyz.openbmc_project.BIOSConfig.Manager."
                        "AttributeType.Enumeration",
                        sysOperatingModeValue));
}

static void bootSideSwitch()
{
    const auto bootSidePaths = utils::getBootSidePaths();

    if (bootSidePaths.size() != 0)
    {
        if (bootSidePaths.size() > 2)
        {
            std::cout << "Received more than two boot paths, Setting the "
                         "first path with priority 1 as next boot path"
                      << std::endl;
        }

        for (const auto& path : bootSidePaths)
        {
            auto retVal = utils::readBusProperty<std::variant<uint8_t>>(
                "xyz.openbmc_project.Software.BMC.Updater", path,
                "xyz.openbmc_project.Software.RedundancyPriority", "Priority");

            if (auto priority = std::get_if<uint8_t>(&retVal))
            {
                if (*priority != 0)
                {
                    uint8_t value = 0;
                    utils::writeBusProperty<uint8_t>(
                        "xyz.openbmc_project.Software.BMC.Updater", path,
                        "xyz.openbmc_project.Software.RedundancyPriority",
                        "Priority", value);

                    // once the value is updated no need to check for other
                    // paths.
                    return;
                }
            }
        }
    }
    std::cerr << "No boot paths returned by mapper call. Hence boot switch "
                 "not executed"
              << std::endl;
}

void Executor::execute02(const types::FunctionalityList& subFuncNumber)
{
    // 127 is sent in sub function number when the state is invalid.
    static constexpr auto invalidState = 127;

    // BIOS table attribute list.
    types::PendingAttributesType listOfAttributeValue;

    // change is needed only when state is not invalid.
    if (subFuncNumber.at(0) != invalidState)
    {
        listOfAttributeValue.push_back(std::make_pair(
            "pvm_os_boot_type",
            std::make_tuple("xyz.openbmc_project.BIOSConfig.Manager."
                            "AttributeType.Enumeration",
                            getIplType(subFuncNumber.at(0)))));
    }

    if (subFuncNumber.at(1) != invalidState)
    {
        listOfAttributeValue.push_back(setOperatingMode(subFuncNumber.at(1)));
    }

    // Process boot side switch only when state is not invalid. Implies
    // change required.
    if (subFuncNumber.at(2) != invalidState)
    {
        // switch boot side.
        bootSideSwitch();
    }

    if (listOfAttributeValue.size() > 0)
    {
        utils::writeBusProperty<types::PendingAttributesType>(
            "xyz.openbmc_project.BIOSConfigManager",
            "/xyz/openbmc_project/bios_config/manager",
            "xyz.openbmc_project.BIOSConfig.Manager", "PendingAttributes",
            std::move(listOfAttributeValue));
    }
}

void Executor::storeIPLSRC(const std::string& progressCode)
{
    // Need to store last 25 IPL SRCs.
    if (iplSrcs.size() == 25)
    {
        iplSrcs.pop_front();
    }
    iplSrcs.push_back(progressCode);
}

void Executor::execute63(const types::FunctionNumber subFuncNumber)
{
    // 0th Sub function is always enabled and should show blank screen if
    // required.
    if ((subFuncNumber == 0) && (iplSrcs.size() == 0))
    {
        utils::sendCurrDisplayToPanel(std::string{}, std::string{}, transport);
        return;
    }
    else
    {
        if ((iplSrcs.size() - 1) >= subFuncNumber)
        {
            utils::sendCurrDisplayToPanel(iplSrcs.at(subFuncNumber),
                                          std::string{}, transport);
            return;
        }
    }

    std::cerr << "Sub function number should not have been enabled"
              << std::endl;
}

void Executor::storePelEventId(const std::string& pelEventId)
{
    // Need to store last 25 PEL SRCs.
    if (pelEventIdQueue.size() == 25)
    {
        pelEventIdQueue.pop_front();
    }
    pelEventIdQueue.push_back(pelEventId);
}

void Executor::execute64(const types::FunctionNumber subFuncNumber)
{
    // 0th Sub function is always enabled and should show blank screen if
    // required.
    if ((subFuncNumber == 0) && (pelEventIdQueue.size() == 0))
    {
        utils::sendCurrDisplayToPanel(std::string{}, std::string{}, transport);
        return;
    }
    else
    {
        if ((pelEventIdQueue.size() - 1) >= subFuncNumber)
        {
            std::string_view src(pelEventIdQueue.at(subFuncNumber));
            if (src.length() < 8)
            {
                std::cerr << "Bad error event data" << std::endl;
                return;
            }
            // TODO: via https://github.com/ibm-openbmc/ibm-panel/issues/34.
            // avoid temp string object creation by using a std::string_view
            utils::sendCurrDisplayToPanel(std::string{src.substr(0, 8)},
                                          std::string{}, transport);
            return;
        }
    }

    std::cerr << "Sub function number should not have been enabled"
              << std::endl;
}

void Executor::execute55(const types::FunctionalityList& subFuncNumber)
{
    /** dump policy: true(01), false(02) */
    if (subFuncNumber.at(0) == 0x00) // view dump policy
    {
        auto result = utils::readBusProperty<std::variant<bool>>(
            "xyz.openbmc_project.Settings",
            "/xyz/openbmc_project/dump/system_dump_policy",
            "xyz.openbmc_project.Object.Enable", "Enabled");

        if (auto val = std::get_if<bool>(&result))
        {
            std::string line1 = "5500 ";
            line1 += *val ? "01" : "02";
            utils::sendCurrDisplayToPanel(line1, "", transport);
            return;
        }
        else
        {
            throw FunctionFailure("Dump policy collection failed.");
        }
    }
    else if (subFuncNumber.at(0) == 0x01) // disable dump policy
    {
        utils::writeBusProperty<bool>(
            "xyz.openbmc_project.Settings",
            "/xyz/openbmc_project/dump/system_dump_policy",
            "xyz.openbmc_project.Object.Enable", "Enabled", false);
    }
    else if (subFuncNumber.at(0) == 0x02) // enable dump policy
    {
        utils::writeBusProperty<bool>(
            "xyz.openbmc_project.Settings",
            "/xyz/openbmc_project/dump/system_dump_policy",
            "xyz.openbmc_project.Object.Enable", "Enabled", true);
    }
    else
    {
        throw FunctionFailure("Function 55 failed. Unsupported sub function.");
    }

    displayExecutionStatus(55, subFuncNumber, true);
}

void Executor::execute08()
{
    // set the transition state of chassis to poweroff.
    panel::utils::writeBusProperty<std::string>(
        "xyz.openbmc_project.State.Chassis",
        "/xyz/openbmc_project/state/chassis0",
        "xyz.openbmc_project.State.Chassis", "RequestedPowerTransition",
        "xyz.openbmc_project.State.Chassis.Transition.Off");

    utils::sendCurrDisplayToPanel("SHUTDOWN SERVER", "INITIATED", transport);
}

static void createDump(const sdbusplus::message::object_path& object)
{
    sdbusplus::message::object_path retVal{};
    auto bus = sdbusplus::bus::new_default();
    auto properties = bus.new_method_call(
        "xyz.openbmc_project.Dump.Manager", std::string(object).c_str(),
        "xyz.openbmc_project.Dump.Create", "CreateDump");
    properties.append(
        std::vector<
            std::pair<std::string, std::variant<std::string, uint64_t>>>());
    auto result = bus.call(properties);
    result.read(retVal);
    std::cout << "Dump initiated. " << std::string(retVal) << std::endl;
}

void Executor::execute43()
{
    createDump(
        sdbusplus::message::object_path("/xyz/openbmc_project/dump/bmc"));
    displayExecutionStatus(43, std::vector<types::FunctionNumber>(), true);
}

void Executor::execute42()
{
    createDump(
        sdbusplus::message::object_path("/xyz/openbmc_project/dump/system"));
    displayExecutionStatus(42, std::vector<types::FunctionNumber>(), true);
}

void Executor::execute04()
{
    utils::writeBusProperty<bool>("xyz.openbmc_project.LED.GroupManager",
                                  "/xyz/openbmc_project/led/groups/lamp_test",
                                  "xyz.openbmc_project.Led.Group", "Asserted",
                                  true);
    utils::doLampTest(transport);
}

static void doBMCGracefulRestart()
{
    utils::writeBusProperty<std::string>(
        "xyz.openbmc_project.State.BMC", "/xyz/openbmc_project/state/bmc0",
        "xyz.openbmc_project.State.BMC", "RequestedBMCTransition",
        "xyz.openbmc_project.State.BMC.Transition.Reboot");
}

void Executor::execute73()
{
    // factory reset BMC by calling
    // BMC code updater factory reset followed by a BMC reboot.
    auto bus = sdbusplus::bus::new_default();
    auto factoryResetCall =
        bus.new_method_call("xyz.openbmc_project.Software.BMC.Updater",
                            "/xyz/openbmc_project/software",
                            "xyz.openbmc_project.Common.FactoryReset", "Reset");

    bus.call(factoryResetCall);

    // Factory Reset doesn't actually happen until a reboot
    doBMCGracefulRestart();
}

void Executor::sendFuncNumToPhyp(const types::FunctionNumber& funcNumber)
{
    PldmFramework obj;
    obj.sendPanelFunctionToPhyp(funcNumber);
    displayExecutionStatus(funcNumber, std::vector<types::FunctionNumber>(),
                           true);
}

void Executor::execute74()
{
    static boost::asio::steady_timer timeout(*io_context);
    // timer for 30 minutes
    auto asyncCancelled = timeout.expires_after(std::chrono::minutes(30));

    (asyncCancelled == 0) ? std::cout << "Timer started" << std::endl
                          : std::cout << "Timer re-started" << std::endl;

    timeout.async_wait([this](const boost::system::error_code& ec) {
        if (ec == boost::asio::error::operation_aborted)
        {
            return;
        }

        if (ec)
        {
            std::cerr << "Timer wait failed for function 74" << ec.message();
        }
        iface->set_property("ACFWindowActive", false);
    });

    if (!asyncCancelled)
    {
        if (!iface->set_property("ACFWindowActive", true))
        {
            timeout.cancel();
            throw FunctionFailure("Failed to set the window property");
        }
    }
    displayExecutionStatus(74, std::vector<types::FunctionNumber>(), true);
}

} // namespace panel
