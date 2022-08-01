#include "utils.hpp"

#include "const.hpp"
#include "exception.hpp"
#include "i2c_message_encoder.hpp"

#include <libpldm/platform.h>

namespace panel
{
namespace utils
{
// Global variables to restore state of display lines.
std::string restoreLine1, restoreLine2;

std::string binaryToHexString(const types::Binary& val)
{
    std::ostringstream oss;
    for (auto i : val)
    {
        oss << std::setw(2) << std::setfill('0') << std::hex
            << static_cast<int>(i);
    }
    return oss.str();
}

void createPEL(const std::string& errIntf, const std::string& sev,
               const std::map<std::string, std::string>& additionalData)
{
    try
    {
        auto bus = sdbusplus::bus::new_default();
        auto service = getService(bus, constants::loggerObjectPath,
                                  constants::loggerCreateInterface);
        auto method =
            bus.new_method_call(service.c_str(), constants::loggerObjectPath,
                                constants::loggerCreateInterface, "Create");

        method.append(errIntf, sev, additionalData);
        bus.call(method);
    }
    catch (const sdbusplus::exception::exception& e)
    {
        std::cerr << "Error in invoking D-Bus logging create interface to "
                     "register PEL";
    }
}

std::string getService(sdbusplus::bus::bus& bus, const std::string& path,
                       const std::string& interface)
{
    auto mapper = bus.new_method_call(constants::mapperDestination,
                                      constants::mapperObjectPath,
                                      constants::mapperInterface, "GetObject");
    mapper.append(path, std::vector<std::string>({interface}));

    std::map<std::string, std::vector<std::string>> response;
    try
    {
        auto reply = bus.call(mapper);
        reply.read(response);
    }
    catch (const sdbusplus::exception::exception& e)
    {
        throw std::runtime_error("Service name is not found");
    }

    if (response.empty())
    {
        throw std::runtime_error("Service name response is empty");
    }

    return response.begin()->first;
}

void sendCurrDisplayToPanel(const std::string& line1, const std::string& line2,
                            std::shared_ptr<Transport> transport)
{
    // TODO: via https://github.com/ibm-openbmc/ibm-panel/issues/37
    // Make these couts and other traces in the code configurable. Keeping
    // these commented until then as they flood the systemd journal.
    // couts are for debugging purpose. can be removed once the testing is done.
    // std::cout << "L1 : " << line1 << std::endl;
    // std::cout << "L2 : " << line2 << std::endl;

    // Restore the values of display lines
    restoreLine1 = line1;
    restoreLine2 = line2;

    encoder::MessageEncoder encode;

    auto displayPacket = encode.rawDisplay(line1, line2);

    transport->panelI2CWrite(displayPacket);

    if ((line1.length() > 16) && (line2.length() > 16))
    {
        transport->panelI2CWrite(encode.scroll(
            static_cast<types::Byte>(types::ScrollType::BOTH_LEFT)));
    }
    else if (line1.length() > 16)
    {
        transport->panelI2CWrite(encode.scroll(
            static_cast<types::Byte>(types::ScrollType::LINE1_LEFT)));
    }
    else if (line2.length() > 16)
    {
        transport->panelI2CWrite(encode.scroll(
            static_cast<types::Byte>(types::ScrollType::LINE2_LEFT)));
    }
}

void readSystemOperatingMode(std::string& sysOperatingMode)
{
    auto readRestorePolicy = readBusProperty<std::variant<std::string>>(
        "xyz.openbmc_project.Settings",
        "/xyz/openbmc_project/control/host0/power_restore_policy",
        "xyz.openbmc_project.Control.Power.RestorePolicy",
        "PowerRestorePolicy");

    auto readRebootPolicy = readBusProperty<std::variant<bool>>(
        "xyz.openbmc_project.Settings",
        "/xyz/openbmc_project/control/host0/auto_reboot",
        "xyz.openbmc_project.Control.Boot.RebootPolicy", "AutoReboot");

    const auto restorePolicy = std::get_if<std::string>(&readRestorePolicy);
    const auto autoRebootPolicy = std::get_if<bool>(&readRebootPolicy);

    if (restorePolicy != nullptr && autoRebootPolicy != nullptr)
    {
        if (*restorePolicy == "xyz.openbmc_project.Control.Power."
                              "RestorePolicy.Policy.AlwaysOff" &&
            *autoRebootPolicy == false)
        {
            sysOperatingMode = "Manual";
        }
        else
        {
            sysOperatingMode = "Normal";
        }
    }
    else
    {
        std::cerr << "Failed to read Bus property" << std::endl;
    }
}

types::SystemParameterValues readSystemParameters()
{
    auto retVal = readBusProperty<std::variant<types::BiosBaseTable>>(
        "xyz.openbmc_project.BIOSConfigManager",
        "/xyz/openbmc_project/bios_config/manager",
        "xyz.openbmc_project.BIOSConfig.Manager", "BaseBIOSTable");

    const auto baseBiosTable = std::get_if<types::BiosBaseTable>(&retVal);

    // system parameters to be read from BIOS table
    std::string OSBootType{};
    std::string HMCManaged{};
    std::string FWIPLType{};
    std::string hypType{};
    std::string systemOperatingMode{};

    if (baseBiosTable != nullptr)
    {
        for (const types::BiosBaseTableItem& item : *baseBiosTable)
        {
            const auto attributeName = std::get<0>(item);
            const auto attrValue = std::get<5>(std::get<1>(item));
            const auto val = std::get_if<std::string>(&attrValue);

            if (val != nullptr)
            {
                // TODO: How to get the information from PLDM if IPL type is
                // enabled to be displayed. Based on that execution of
                // function 01 needs to be updated to display this data.
                // Currently it is disabled in the code explicitly.
                if (attributeName == "pvm_os_boot_type")
                {
                    OSBootType = *val;
                }
                else if (attributeName == "pvm_hmc_managed")
                {
                    HMCManaged = *val;
                }
                else if (attributeName == "hb_hyp_switch")
                {
                    hypType = *val;
                }
            }
        }
    }
    else
    {
        std::cerr << "Failed to read BIOS base table" << std::endl;
    }

    readSystemOperatingMode(systemOperatingMode);

    return std::make_tuple(OSBootType, systemOperatingMode, HMCManaged,
                           FWIPLType, hypType);
}

types::GetManagedObjects getManagedObjects(const std::string& service,
                                           const std::string& object)
{
    types::GetManagedObjects retVal{};
    try
    {
        auto bus = sdbusplus::bus::new_default();
        auto properties = bus.new_method_call(
            service.c_str(), object.c_str(),
            "org.freedesktop.DBus.ObjectManager", "GetManagedObjects");
        auto result = bus.call(properties);
        result.read(retVal);
    }
    catch (const sdbusplus::exception::SdBusError& e)
    {
        std::cerr << e.what();
    }
    return retVal;
}

void getNextBootSide(std::string& nextBootSide)
{
    std::string valueType;
    std::variant<std::string> bootSideValue;
    std::variant<std::string> var3;

    auto bus = sdbusplus::bus::new_default();
    auto method = bus.new_method_call(
        "xyz.openbmc_project.BIOSConfigManager",
        "/xyz/openbmc_project/bios_config/manager",
        "xyz.openbmc_project.BIOSConfig.Manager", "GetAttribute");
    method.append("fw_boot_side");
    auto result = bus.call(method);
    result.read(valueType, bootSideValue, var3);

    if (auto bootSide = std::get_if<std::string>(&bootSideValue))
    {
        if (*bootSide == "Perm")
        {
            nextBootSide = "P";
        }
        else
        {
            nextBootSide = "T";
        }
    }
}

void doLampTest(std::shared_ptr<Transport>& transport)
{
    transport->panelI2CWrite(encoder::MessageEncoder().lampTest());
    std::cout << "\nPanel lamp test initiated." << std::endl;
}

void restoreDisplayOnPanel(std::shared_ptr<Transport>& transport)
{
    sendCurrDisplayToPanel(restoreLine1, restoreLine2, transport);
}

types::PdrList getPDR(const uint8_t& terminusId, const uint16_t& entityId,
                      const uint16_t& stateSetId, const std::string& pdrMethod)
{
    types::PdrList pdrs{};
    try
    {
        auto bus = sdbusplus::bus::new_default();
        auto method = bus.new_method_call(
            "xyz.openbmc_project.PLDM", "/xyz/openbmc_project/pldm",
            "xyz.openbmc_project.PLDM.PDR", pdrMethod.c_str());
        method.append(terminusId, entityId, stateSetId);
        auto responseMsg = bus.call(method);
        responseMsg.read(pdrs);
    }
    catch (const sdbusplus::exception::SdBusError& e)
    {
        std::cerr << e.what() << std::endl;
        throw FunctionFailure("pldm: Failed to fetch the PDR.");
    }
    return pdrs;
}

void getSensorDataFromPdr(const types::PdrList& stateSensorPdr,
                          uint16_t& sensorId)
{
    auto pdr = reinterpret_cast<const pldm_state_sensor_pdr*>(
        stateSensorPdr.front().data());
    sensorId = pdr->sensor_id;
}

std::vector<std::string> getSubTreePaths(const std::string& objectPath,
                                         const std::vector<std::string>& intf,
                                         const int32_t depth)
{
    std::vector<std::string> result;

    try
    {
        auto bus = sdbusplus::bus::new_default();
        auto mapperCall = bus.new_method_call(
            "xyz.openbmc_project.ObjectMapper",
            "/xyz/openbmc_project/object_mapper",
            "xyz.openbmc_project.ObjectMapper", "GetSubTreePaths");

        mapperCall.append(objectPath);
        mapperCall.append(depth);
        mapperCall.append(intf);

        auto response = bus.call(mapperCall);
        response.read(result);
    }
    catch (const sdbusplus::exception::SdBusError& e)
    {
        std::cerr << e.what();
    }

    return result;
}

std::string getSystemIM()
{
    auto im = readBusProperty<std::variant<types::Binary>>(
        constants::inventoryManagerIntf, constants::systemDbusObj,
        constants::imInterface, constants::imKeyword);
    if (auto imVector = std::get_if<types::Binary>(&im))
    {
        return utils::binaryToHexString(*imVector);
    }
    else
    {
        std::cerr << "\n Failed querying IM property from dbus" << std::endl;
    }
    return "";
}

bool getLcdPanelPresentProperty(const std::string& imValue)
{
    auto present = readBusProperty<std::variant<bool>>(
        constants::inventoryManagerIntf,
        std::get<2>((lcdDataMap.find(imValue))->second),
        constants::itemInterface, "Present");
    if (auto p = std::get_if<bool>(&present))
    {
        return *p;
    }
    else
    {
        std::cerr << "\n Failed querying Present property from dbus."
                  << std::endl;
    }
    return false;
}

} // namespace utils
} // namespace panel
