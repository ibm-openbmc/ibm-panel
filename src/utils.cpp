#include "utils.hpp"

#include "i2c_message_encoder.hpp"

namespace panel
{
namespace utils
{
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

void sendCurrDisplayToPanel(const std::string& line1, const std::string& line2,
                            std::shared_ptr<Transport> transport)
{
    // TODO: via https://github.com/ibm-openbmc/ibm-panel/issues/37
    // Make these couts and other traces in the code configurable. Keeping
    // these commented until then as they flood the systemd journal.
    // couts are for debugging purpose. can be removed once the testing is done.
    // std::cout << "L1 : " << line1 << std::endl;
    // std::cout << "L2 : " << line2 << std::endl;

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
    auto readLogSettings = readBusProperty<std::variant<bool>>(
        "xyz.openbmc_project.Settings", "/xyz/openbmc_project/logging/settings",
        "xyz.openbmc_project.Logging.Settings", "QuiesceOnHwError");

    auto readRestorePolicy = readBusProperty<std::variant<std::string>>(
        "xyz.openbmc_project.Settings",
        "/xyz/openbmc_project/control/host0/power_restore_policy",
        "xyz.openbmc_project.Control.Power.RestorePolicy",
        "PowerRestorePolicy");

    auto readRebootPolicy = readBusProperty<std::variant<bool>>(
        "xyz.openbmc_project.Settings",
        "/xyz/openbmc_project/control/host0/auto_reboot",
        "xyz.openbmc_project.Control.Boot.RebootPolicy", "AutoReboot");

    const auto loggingService = std::get_if<bool>(&readLogSettings);
    const auto restorePolicy = std::get_if<std::string>(&readRestorePolicy);
    const auto autoRebootPolicy = std::get_if<bool>(&readRebootPolicy);

    if (loggingService != nullptr && restorePolicy != nullptr &&
        autoRebootPolicy != nullptr)
    {
        if (*loggingService == true &&
            *restorePolicy == "xyz.openbmc_project.Control.Power."
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
                // TODO: needs to be updated. Now phyp does not use P/T.
                // BIOS table will not be used for this. Will be stored at the
                // panel end.
                // Have to come up woth new terminology. BMC does not
                // have P/T boot side.
                // To Check: When user updates the value to
                // backup image using function 02 if the display of function 01
                // does not changes accordingly then we do not need to show this
                // at all in function 01.
                else if (attributeName == "pvm_fw_boot_side")
                {
                    FWIPLType = *val;
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

std::vector<std::string> getBootSidePaths()
{
    std::vector<std::string> result;
    result.reserve(2);

    auto bus = sdbusplus::bus::new_default();
    auto mapperCall = bus.new_method_call("xyz.openbmc_project.ObjectMapper",
                                          "/xyz/openbmc_project/object_mapper",
                                          "xyz.openbmc_project.ObjectMapper",
                                          "GetSubTreePaths");

    const int32_t depth = 0;
    std::vector<std::string> intf{
        "xyz.openbmc_project.Software.RedundancyPriority"};

    mapperCall.append("/xyz/openbmc_project/software");
    mapperCall.append(depth);
    mapperCall.append(intf);

    auto response = bus.call(mapperCall);
    response.read(result);

    // any bus exception will be caught in state manager.
    return result;
}

void getNextBootSide(std::string& nextBootSide)
{
    auto bootSidePaths = getBootSidePaths();

    // If we do not receive any path or just one path we default current
    // boot side as "P".
    if (bootSidePaths.size() == 2)
    {
        // get functional framework list
        auto res =
            utils::readBusProperty<std::variant<std::vector<std::string>>>(
                "xyz.openbmc_project.ObjectMapper",
                "/xyz/openbmc_project/software/functional",
                "xyz.openbmc_project.Association", "endpoints");

        const auto functionalFw = std::get_if<std::vector<std::string>>(&res);
        if ((functionalFw == nullptr) || (functionalFw->size() == 0))
        {
            // We could not get any functional FW. This should not be a
            // situation. log error.
            throw std::runtime_error("Error fetching functionalFw");
        }

        std::cout << "Functional Image size = " << functionalFw->size()
                  << std::endl;

        bool runningImageFound = false;
        std::string runningImagePath{};

        for (const auto& item : *functionalFw)
        {
            auto pos =
                std::find(bootSidePaths.begin(), bootSidePaths.end(), item);

            if (pos != bootSidePaths.end())
            {
                runningImagePath = *pos;
                runningImageFound = true;
                std::cout << "Running image found" << runningImagePath
                          << std::endl;
                break;
            }
        }

        if (!runningImageFound)
        {
            throw std::runtime_error("Functional fw not found in boot paths");
        }

        auto resp = utils::readBusProperty<std::variant<uint8_t>>(
            "xyz.openbmc_project.Software.BMC.Updater", runningImagePath,
            "xyz.openbmc_project.Software.RedundancyPriority", "Priority");

        if (const auto imagePriority = std::get_if<uint8_t>(&resp);
            (imagePriority != nullptr))
        {
            // implies this is the running image and it is also marked for next
            // boot.
            if (*imagePriority == 0)
            {
                nextBootSide = "P";
            }
            // implies running image is not marked for next boot.
            else if (*imagePriority == 1)
            {
                nextBootSide = "T";
            }
        }
        else
        {
            throw std::runtime_error("Failed to read boot priority property");
        }
    }
    else
    {
        std::cout << "Boot side path not equal to 2. Always mark "
                     "selected side as P"
                  << std::endl;
    }
}

void doLampTest(std::shared_ptr<Transport>& transport)
{
    transport->panelI2CWrite(encoder::MessageEncoder().lampTest());
    std::cout << "\nPanel lamp test initiated." << std::endl;
}

} // namespace utils
} // namespace panel