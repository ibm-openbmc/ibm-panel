#include "bus_monitor.hpp"

#include "const.hpp"
#include "utils.hpp"

#include <algorithm>
#include <boost/algorithm/string.hpp>
#include <vector>

namespace panel
{

void PanelPresence::readPresentProperty(sdbusplus::message::message& msg)
{
    if (msg.is_method_error())
    {
        std::cerr << "\n Error in reading panel presence signal " << std::endl;
    }
    std::string object;
    types::ItemInterfaceMap invItemMap;
    msg.read(object, invItemMap);
    const auto itr = invItemMap.find("Present");
    if (itr != invItemMap.end())
    {
        if (auto present = std::get_if<bool>(&(itr->second)))
        {
            transport->setTransportKey(*present);
        }
        else
        {
            std::cerr << "\n Error reading panel present property" << std::endl;
        }
    }
}

void PanelPresence::listenPanelPresence()
{
    static std::shared_ptr<sdbusplus::bus::match::match> matchPanelPresence =
        std::make_shared<sdbusplus::bus::match::match>(
            *conn,
            sdbusplus::bus::match::rules::propertiesChanged(
                objectPath, constants::itemInterface),
            [this](sdbusplus::message::message& msg) {
                readPresentProperty(msg);
            });
}

void PELListener::PELEventCallBack(sdbusplus::message::message& msg)
{
    // TODO: we need for delete event as well.

    sdbusplus::message::object_path objPath;

    types::DbusInterfaceMap infMap;

    msg.read(objPath, infMap);

    const auto infItr = infMap.find("xyz.openbmc_project.Logging.Entry");
    if (infItr != infMap.end())
    {
        const auto& propMap = infItr->second;

        auto propItr = propMap.find("Severity");
        if (propItr != propMap.end())
        {
            const auto severity = std::get_if<std::string>(&propItr->second);

            // TODO: need to check which all severty needs to be taken care
            // of
            if (severity != nullptr &&
                *severity !=
                    "xyz.openbmc_project.Logging.Entry.Level.Informational")
            {
                types::FunctionalityList list;
                // as there are maximum 9 SRC related functions.
                list.reserve(9);

                if (!functionStateEnabled)
                {
                    // these functions needs to be enabled only once when first
                    // PEL of desired severity is received.
                    functionStateEnabled = true;
                    list.emplace_back(11);
                    list.emplace_back(12);
                    list.emplace_back(13);
                }

                propItr = propMap.find("Resolution");
                if (propItr != propMap.end())
                {
                    const auto resolution =
                        std::get_if<std::string>(&propItr->second);
                    if (resolution != nullptr || !(*resolution).empty())
                    {
                        std::vector<std::string> callOutList;
                        boost::split(callOutList, *resolution,
                                     boost::is_any_of("\n"));

                        // Need to show max 6 callout src.
                        auto size = std::min(callOutList.size(),
                                             static_cast<size_t>(6));

                        // default list: 14 to 19 are the functions to display
                        // callout SRCs.
                        constexpr std::array<types::FunctionNumber, 6>
                            callOutSRCFunctions{14, 15, 16, 17, 18, 19};

                        // add functions to enable list based on number of
                        // callouts.
                        list.insert(std::end(list), callOutSRCFunctions.begin(),
                                    callOutSRCFunctions.begin() + size);

                        // Need to also disable functions in the range of 14 to
                        // 19 based on number of callouts.
                        if (size < callOutSRCFunctions.size())
                        {
                            // disable rest of the functions
                            types::FunctionalityList disableFunc(
                                (callOutSRCFunctions.begin() + size),
                                callOutSRCFunctions.end());

                            stateManager->disableFunctonality(disableFunc);
                        }

                        executor->pelCallOutList(callOutList);
                    }
                    else
                    {
                        std::cout << "No Callout found in the PEL";
                    }
                }

                if (list.size() > 0)
                {
                    stateManager->enableFunctonality(list);
                }

                propItr = propMap.find("EventId");
                if (propItr != propMap.end())
                {
                    if (const auto eventId =
                            std::get_if<std::string>(&propItr->second))
                    {
                        executor->storePelEventId(*eventId);
                        return;
                    }
                }
                std::cerr << "Event ID property not found" << std::endl;
            }
        }
    }
}

void PELListener::listenPelEvents()
{
    static auto sigMatch = std::make_unique<sdbusplus::bus::match::match>(
        *conn,
        sdbusplus::bus::match::rules::interfacesAdded(
            "/xyz/openbmc_project/logging"),
        [this](sdbusplus::message::message& msg) { PELEventCallBack(msg); });
}

void BootProgressCode::listenProgressCode()
{
    // signal match for sdbusplus
    static auto sigMatch = std::make_unique<sdbusplus::bus::match::match>(
        *conn,
        sdbusplus::bus::match::rules::propertiesChanged(
            "/xyz/openbmc_project/state/boot/raw0",
            "xyz.openbmc_project.State.Boot.Raw"),
        [this](sdbusplus::message::message& msg) {
            progressCodeCallBack(msg);
        });
}

void BootProgressCode::progressCodeCallBack(sdbusplus::message::message& msg)
{
    using PostCode = std::tuple<uint64_t, std::vector<types::Byte>>;

    std::string interface{};
    std::map<std::string, std::variant<PostCode>> propertyMap;

    msg.read(interface, propertyMap);

    // property we are looking for.
    const auto it = propertyMap.find("Value");
    if (it != propertyMap.end())
    {
        if (auto postCodeData = std::get_if<PostCode>(&(it->second)))
        {
            auto src = std::get<0>(*postCodeData);

            // clear display if progress code ascii equals to "00000000"
            if (src == constants::clearDisplayProgressCode)
            {
                utils::sendCurrDisplayToPanel(std::string{}, std::string{},
                                              transport);
                return;
            }

            std::vector<types::Byte> byteArray;
            byteArray.reserve(sizeof(src));

            for (size_t i = 0; i < sizeof(src); i++)
            {
                byteArray.emplace_back(types::Byte(src >> (sizeof(src) * i)) &
                                       0xFF);
            }

            utils::sendCurrDisplayToPanel(
                std::string(byteArray.begin(), byteArray.end()), std::string{},
                transport);

            executor->storeIPLSRC(
                std::string(byteArray.begin(), byteArray.end()));
        }
        else
        {
            std::cerr << "Progress code Data error" << std::endl;
        }
    }
}

SystemStatus::SystemStatus(
    std::shared_ptr<sdbusplus::asio::connection>& con,
    std::shared_ptr<state::manager::PanelStateManager>& manager) :
    conn(con),
    stateManager(manager)
{
    initSystemOperatingParameters();
    listenBmcState();
    listenBootProgressState();
    listenPowerState();
    listenSystemOperatingModeParameters();
}

void SystemStatus::bmcStateCallback(sdbusplus::message::message& msg)
{
    std::string object{};
    types::ItemInterfaceMap invItemMap;

    msg.read(object, invItemMap);
    const auto itr = invItemMap.find("CurrentBMCState");
    if (itr != invItemMap.end())
    {
        if (auto bmcState = std::get_if<std::string>(&(itr->second)))
        {
            // std::cout << "BMC state = " << *bmcState << std::endl;
            stateManager->updateBMCState(*bmcState);
        }
        else
        {
            std::cerr << "\n Error reading bmc state property" << std::endl;
        }
    }
}

void SystemStatus::listenBmcState()
{
    // Before registering signal read the current value
    auto retCurBmcState = utils::readBusProperty<std::variant<std::string>>(
        "xyz.openbmc_project.State.BMC", "/xyz/openbmc_project/state/bmc0",
        "xyz.openbmc_project.State.BMC", "CurrentBMCState");

    if (auto curBmcState = std::get_if<std::string>(&retCurBmcState))
    {
        stateManager->updateBMCState(*curBmcState);
    }
    else
    {
        // read failed for current bmc state so set it as "not ready".
        stateManager->updateBMCState(
            "xyz.openbmc_project.State.BMC.BMCState.NotReady");
    }

    static auto sigBmcState = std::make_unique<sdbusplus::bus::match::match>(
        *conn,
        sdbusplus::bus::match::rules::propertiesChanged(
            "/xyz/openbmc_project/state/bmc0", "xyz.openbmc_project.State.BMC"),
        [this](sdbusplus::message::message& msg) { bmcStateCallback(msg); });
}

void SystemStatus::powerStateCallback(sdbusplus::message::message& msg)
{
    std::string object{};
    types::ItemInterfaceMap invItemMap;

    msg.read(object, invItemMap);
    const auto itr = invItemMap.find("CurrentPowerState");
    if (itr != invItemMap.end())
    {
        if (auto powerState = std::get_if<std::string>(&(itr->second)))
        {
            // std::cout << "Power state = " << *powerState << std::endl;
            stateManager->updatePowerState(*powerState);
        }
        else
        {
            std::cerr << "\n Error reading power state property" << std::endl;
        }
    }
}

void SystemStatus::listenPowerState()
{
    // Before registering signal read the current value
    auto retCurPowerState = utils::readBusProperty<std::variant<std::string>>(
        "xyz.openbmc_project.State.Chassis",
        "/xyz/openbmc_project/state/chassis0",
        "xyz.openbmc_project.State.Chassis", "CurrentPowerState");

    if (auto curPowerState = std::get_if<std::string>(&retCurPowerState))
    {
        stateManager->updatePowerState(*curPowerState);
    }
    else
    {
        // read failed for power state so set it as "Off".
        stateManager->updatePowerState(
            "xyz.openbmc_project.State.Chassis.PowerState.Off");
    }

    static auto sigPowerState = std::make_unique<sdbusplus::bus::match::match>(
        *conn,
        sdbusplus::bus::match::rules::propertiesChanged(
            "/xyz/openbmc_project/state/chassis0",
            "xyz.openbmc_project.State.Chassis"),
        [this](sdbusplus::message::message& msg) { powerStateCallback(msg); });
}

void SystemStatus::bootProgressStateCallback(sdbusplus::message::message& msg)
{
    std::string object{};
    types::ItemInterfaceMap invItemMap;

    msg.read(object, invItemMap);
    const auto itr = invItemMap.find("BootProgress");
    if (itr != invItemMap.end())
    {
        if (auto bootProgressState = std::get_if<std::string>(&(itr->second)))
        {
            // std::cout << "boot progress state = " << *bootProgressState
            //          << std::endl;
            stateManager->updateBootProgressState(*bootProgressState);
        }
        else
        {
            std::cerr << "\n Error reading boot progress state property"
                      << std::endl;
        }
    }
}

void SystemStatus::listenBootProgressState()
{
    // Before registering signal read the current value
    auto retBootProgress = utils::readBusProperty<std::variant<std::string>>(
        "xyz.openbmc_project.State.Host", "/xyz/openbmc_project/state/host0",
        "xyz.openbmc_project.State.Boot.Progress", "BootProgress");

    if (auto curBootProgressState = std::get_if<std::string>(&retBootProgress))
    {
        stateManager->updateBootProgressState(*curBootProgressState);
    }
    else
    {
        // read failed for boot progress state so set it as "Unspecified".
        stateManager->updateBootProgressState(
            "xyz.openbmc_project.State.Boot.Progress."
            "ProgressStages.Unspecified");
    }

    static auto sigBootState = std::make_unique<sdbusplus::bus::match::match>(
        *conn,
        sdbusplus::bus::match::rules::propertiesChanged(
            "/xyz/openbmc_project/state/host0",
            "xyz.openbmc_project.State.Boot.Progress"),
        [this](sdbusplus::message::message& msg) {
            bootProgressStateCallback(msg);
        });
}

void SystemStatus::loggingSettingStateCallback(sdbusplus::message::message& msg)
{
    std::string object{};
    types::ItemInterfaceMap invItemMap;

    msg.read(object, invItemMap);
    const auto itr = invItemMap.find("QuiesceOnHwError");
    if (itr != invItemMap.end())
    {
        if (auto loggingSetting = std::get_if<bool>(&(itr->second)))
        {
            // std::cout << "loggingSettingState = " << *loggingSetting
            //          << std::endl;
            loggingPolicy = *loggingSetting;

            setSystemCurrentOperatingMode();
        }
        else
        {
            std::cerr << "\n Error reading logging state property" << std::endl;
        }
    }
}

void SystemStatus::powerPolicyStateCallback(sdbusplus::message::message& msg)
{
    std::string object{};
    types::ItemInterfaceMap invItemMap;

    msg.read(object, invItemMap);
    const auto itr = invItemMap.find("PowerRestorePolicy");
    if (itr != invItemMap.end())
    {
        if (auto powerState = std::get_if<std::string>(&(itr->second)))
        {
            // std::cout << "power state = " << *powerState << std::endl;
            powerPolicy = *powerState;

            setSystemCurrentOperatingMode();
        }
        else
        {
            std::cerr << "Failed to read power policy from Dbus" << std::endl;
        }
    }
}

void SystemStatus::rebootPolicyStateCallback(sdbusplus::message::message& msg)
{
    std::string object{};
    types::ItemInterfaceMap invItemMap;

    msg.read(object, invItemMap);
    const auto itr = invItemMap.find("AutoReboot");
    if (itr != invItemMap.end())
    {
        if (auto rebootState = std::get_if<bool>(&(itr->second)))
        {
            // std::cout << "reboot state = " << *rebootState << std::endl;
            rebootPolicy = *rebootState;

            setSystemCurrentOperatingMode();
        }
        else
        {
            std::cerr << "Failed to read reboot policy from Dbus" << std::endl;
        }
    }
}

void SystemStatus::listenSystemOperatingModeParameters()
{
    static auto sigLoggingSettings =
        std::make_unique<sdbusplus::bus::match::match>(
            *conn,
            sdbusplus::bus::match::rules::propertiesChanged(
                "/xyz/openbmc_project/logging/settings",
                "xyz.openbmc_project.Logging.Settings"),
            [this](sdbusplus::message::message& msg) {
                loggingSettingStateCallback(msg);
            });

    static auto sigPowerPolicy = std::make_unique<sdbusplus::bus::match::match>(
        *conn,
        sdbusplus::bus::match::rules::propertiesChanged(
            "/xyz/openbmc_project/control/host0/power_restore_policy",
            "xyz.openbmc_project.Control.Power.RestorePolicy"),
        [this](sdbusplus::message::message& msg) {
            powerPolicyStateCallback(msg);
        });

    static auto sigRebootPolicy =
        std::make_unique<sdbusplus::bus::match::match>(
            *conn,
            sdbusplus::bus::match::rules::propertiesChanged(
                "/xyz/openbmc_project/control/host0/auto_reboot",
                "xyz.openbmc_project.Control.Boot.RebootPolicy"),
            [this](sdbusplus::message::message& msg) {
                rebootPolicyStateCallback(msg);
            });
}

void SystemStatus::initSystemOperatingParameters()
{
    auto retlogSettings = utils::readBusProperty<std::variant<bool>>(
        "xyz.openbmc_project.Settings", "/xyz/openbmc_project/logging/settings",
        "xyz.openbmc_project.Logging.Settings", "QuiesceOnHwError");

    if (auto logSettings = std::get_if<bool>(&retlogSettings))
    {
        loggingPolicy = *logSettings;
    }
    else
    {
        // for error set the parameters for Normal mode value.
        loggingPolicy = false;
        std::cerr << "Failed to read logging setting dbus property"
                  << std::endl;
    }

    auto retPowerSettings = utils::readBusProperty<std::variant<std::string>>(
        "xyz.openbmc_project.Settings",
        "/xyz/openbmc_project/control/host0/power_restore_policy",
        "xyz.openbmc_project.Control.Power.RestorePolicy",
        "PowerRestorePolicy");

    if (auto powerSettings = std::get_if<std::string>(&retPowerSettings))
    {
        powerPolicy = *powerSettings;
    }
    else
    {
        // for error set the parameters for Normal mode value.
        powerPolicy =
            "xyz.openbmc_project.Control.Power.RestorePolicy.Policy.Restore";
        std::cerr << "Failed to read power policy from Dbus" << std::endl;
    }

    auto retRebootSetting = utils::readBusProperty<std::variant<bool>>(
        "xyz.openbmc_project.Settings",
        "/xyz/openbmc_project/control/host0/auto_reboot",
        "xyz.openbmc_project.Control.Boot.RebootPolicy", "AutoReboot");

    if (auto rebootSettings = std::get_if<bool>(&retRebootSetting))
    {
        rebootPolicy = *rebootSettings;
    }
    else
    {
        // for error set the parameters for Normal mode value.
        rebootPolicy = true;
        std::cerr << "Failed t read reboot folicy form Dbus" << std::endl;
    }

    setSystemCurrentOperatingMode();
}

void SystemStatus::setSystemCurrentOperatingMode()
{
    if (loggingPolicy == true &&
        powerPolicy == "xyz.openbmc_project.Control.Power.RestorePolicy.Policy."
                       "AlwaysOff" &&
        rebootPolicy == false)
    {
        std::cout << "System operating mode set to Manual" << std::endl;
        stateManager->setSystemOperatingMode("Manual");
    }
    else
    {
        // if any of the condition fails set mode to normal
        std::cout << "System operating mode set to Normal" << std::endl;
        stateManager->setSystemOperatingMode("Normal");
    }
}
} // namespace panel