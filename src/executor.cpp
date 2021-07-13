#include "executor.hpp"

#include "const.hpp"
#include "utils.hpp"

#include <boost/algorithm/string.hpp>

namespace panel
{

void Executor::executeFunction(const types::FunctionNumber funcNumber,
                               const types::FunctionalityList& subFuncNumber)
{
    // test output, to be removed
    std::cout << funcNumber << std::endl;
    std::cout << subFuncNumber.at(0) << std::endl;

    switch (funcNumber)
    {
        case 1:
            execute01();
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

        case 20:
            execute20();
            break;

        case 30:
            execute30(subFuncNumber);
            break;

        default:
            break;
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

    utils::sendCurrDisplayToPanel(line1, line2, transport);
}

std::string Executor::getSrcDataForPEL()
{
    auto res = utils::readBusProperty<std::variant<std::string>>(
        "xyz.openbmc_project.Logging", pelEventPath,
        "xyz.openbmc_project.Logging.Entry", "EventId");

    auto srcData = std::get_if<std::string>(&res);

    if (srcData != nullptr)
    {
        return *srcData;
    }

    return "";
}

void Executor::execute11()
{
    auto srcData = getSrcDataForPEL();

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
    std::string line2{};
    std::string ethObjPath = constants::networkManagerObj;
    ethObjPath += "/";
    ethObjPath += ethPort;
    std::string macAddr{}, locCode{};

    for (const auto& obj : networkObjects)
    {
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

                        // Ignore link local ip and display the other one
                        // (could be either static/dynamic ip)
                        if ((type != nullptr) && (origin != nullptr) &&
                            (*type == "xyz.openbmc_project.Network.IP."
                                      "Protocol.IPv4") &&
                            (*origin != "xyz.openbmc_project.Network.IP."
                                        "AddressOrigin.LinkLocal"))
                        {
                            const auto& addressItr = propValMap.find("Address");
                            if (addressItr != propValMap.end())
                            {
                                const auto& address = std::get_if<std::string>(
                                    &(addressItr->second));
                                if (address != nullptr)
                                {
                                    line2 = *address;
                                    break;
                                }
                            }
                        }
                    }
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
        if (!macAddr.empty() && !line2.empty())
        {
            break;
        }
    }
    if (line2.empty())
    {
        std::cerr << "\n IP address of the ethernet port is not found"
                  << std::endl;
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

    if (!std::get<0>(sysValues).empty() && !std::get<1>(sysValues).empty() &&
        !std::get<2>(sysValues).empty() && !std::get<3>(sysValues).empty() &&
        !std::get<4>(sysValues).empty())
    {
        std::string line1(16, ' ');
        std::string line2(16, ' ');

        // function number
        line1.replace(0, 2, "01");

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

        // Boot side. This needs to be renamed later.
        line2.replace(12, 1, std::get<3>(sysValues).substr(0, 1));

        utils::sendCurrDisplayToPanel(line1, line2, transport);
        return;
    }

    std::cerr << "Error reading system parameters" << std::endl;
}

void Executor::execute12()
{
    auto srcData = getSrcDataForPEL();

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
    auto srcData = getSrcDataForPEL();

    // Need to show blank spaces in case of no srcData as function is enabled.
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

} // namespace panel