#include "bus_monitor.hpp"

#include "const.hpp"
#include "utils.hpp"

#include <algorithm>
#include <boost/algorithm/string.hpp>
#include <vector>

namespace panel
{

static void resetLEDState()
{
    static constexpr auto ledsPathOnBasePanel = {
        "/xyz/openbmc_project/led/groups/bmc_booted",
        "/xyz/openbmc_project/led/groups/power_on",
        "/xyz/openbmc_project/led/groups/enclosure_identify",
        "/xyz/openbmc_project/led/groups/platform_system_attention_indicator",
        "/xyz/openbmc_project/led/groups/partition_system_attention_indicator",
        "/xyz/openbmc_project/led/groups/enclosure_fault"};

    for (const auto& led : ledsPathOnBasePanel)
    {
        auto res = utils::readBusProperty<std::variant<bool>>(
            "xyz.openbmc_project.LED.GroupManager", led,
            "xyz.openbmc_project.Led.Group", "Asserted");

        if (auto asserted = std::get_if<bool>(&res))
        {
            if (*asserted)
            {
                try
                {
                    utils::writeBusProperty<bool>(
                        "xyz.openbmc_project.LED.GroupManager", led,
                        "xyz.openbmc_project.Led.Group", "Asserted", false);

                    utils::writeBusProperty<bool>(
                        "xyz.openbmc_project.LED.GroupManager", led,
                        "xyz.openbmc_project.Led.Group", "Asserted", true);
                }
                catch (const sdbusplus::exception::SdBusError& e)
                {
                    std::cerr << "Write call for led " << led
                              << " failed with exception " << e.what()
                              << std::endl;
                }
            }
        }
        else
        {
            std::cerr
                << "Failed to read asserted property. Unable to reset led "
                << led << std::endl;
        }
    }
}

void PanelPresence::readBasePresentProperty(sdbusplus::message::message& msg)
{
    if (msg.is_method_error())
    {
        std::cerr << "\n Error in reading base panel presence signal "
                  << std::endl;
    }
    std::string interface;
    types::PropertyValueMap propMap;
    msg.read(interface, propMap);
    const auto itr = propMap.find("Present");
    if (itr != propMap.end())
    {
        if (auto present = std::get_if<bool>(&(itr->second)))
        {
            if (*present)
            {
                resetLEDState();
            }
        }
        else
        {
            std::cerr << "\n Error reading base panel present property"
                      << std::endl;
        }
    }
}

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
            if (transport->getPanelType() == types::PanelType::LCD && *present)
            {

                const auto bmcState =
                    utils::readBusProperty<std::variant<std::string>>(
                        "xyz.openbmc_project.State.BMC",
                        "/xyz/openbmc_project/state/bmc0",
                        "xyz.openbmc_project.State.BMC", "CurrentBMCState");

                if (const auto* bmc = std::get_if<std::string>(&bmcState))
                {
                    if (*bmc == "xyz.openbmc_project.State.BMC.BMCState.Ready")
                    {
                        stateManager->updateBMCState(*bmc);
                    }
                }
                else
                {
                    std::cerr
                        << "Failed reading CurrentBMCState property from D-Bus."
                        << std::endl;
                }
            }
        }
        else
        {
            std::cerr << "\n Error reading panel present property" << std::endl;
        }
    }
}

void PanelPresence::listenPanelPresence()
{
    // Register call back for Everest base panel. Used in case of CM to reset
    // LEDs.
    if (objectPath == constants::everBaseDbusObj)
    {
        static std::shared_ptr<sdbusplus::bus::match::match>
            everBasePanelPresence =
                std::make_shared<sdbusplus::bus::match::match>(
                    *conn,
                    sdbusplus::bus::match::rules::propertiesChanged(
                        objectPath, constants::itemInterface),
                    [this](sdbusplus::message::message& msg) {
                        readBasePresentProperty(msg);
                    });
    }
    else
    {
        static std::shared_ptr<sdbusplus::bus::match::match>
            matchPanelPresence = std::make_shared<sdbusplus::bus::match::match>(
                *conn,
                sdbusplus::bus::match::rules::propertiesChanged(
                    objectPath, constants::itemInterface),
                [this](sdbusplus::message::message& msg) {
                    readPresentProperty(msg);
                });
    }
}

void PELListener::PELEventCallBack(sdbusplus::message::message& msg)
{
    // TODO: we need for delete event as well.

    sdbusplus::message::object_path objPath;

    types::DbusInterfaceMap infMap;
    msg.read(objPath, infMap);

    // we need too handle signal only in case signal is populated for PEL.Entry
    // interface, as this confirms that data for Event ID field has been
    // populated and published.
    if (infMap.find("org.open_power.Logging.PEL.Entry") != infMap.end())
    {
        auto res = utils::readBusProperty<std::variant<std::string>>(
            "xyz.openbmc_project.Logging", objPath,
            "xyz.openbmc_project.Logging.Entry", "Severity");

        if (const auto& severity = std::get_if<std::string>(&res))
        {
            // Need to process PELs with severity non informational.
            if (*severity !=
                "xyz.openbmc_project.Logging.Entry.Level.Informational")
            {
                setPelRelatedFunctionState(objPath);

                res = utils::readBusProperty<std::variant<std::string>>(
                    "xyz.openbmc_project.Logging", objPath,
                    "xyz.openbmc_project.Logging.Entry", "EventId");

                if (const auto& eventId = std::get_if<std::string>(&res))
                {
                    // Terminating src detection.
                    // Terminating bit is the bit 2(starting from 0)
                    // from MSB end (Big Endian) of 5th Hex word.

                    // Length for 5 hex words required is 44 including
                    // spaces btween them.
                    if ((*eventId).length() >=
                        constants::fiveHexWordsWithSpaces)
                    {
                        std::vector<std::string> hexWords;
                        boost::split(hexWords, *eventId, boost::is_any_of(" "));

                        /*Steps used to check for terminating Bit.
                        Eg: 5th Hexword = "A0000000".
                        - Binary equivalent "1010 0000 0000 0000 0000
                        0000 0000 0000"
                        - Picking nibble from MSB - "1010"
                        - Storing as a Byte - "0000 1010"
                        - Bitwise AND with "0000 0010" to check the
                        terminating bit.*/

                        // picking a nibble value from MSB end (Big
                        // Endian) of the hexword and storing as a char.
                        char valueAtIndexZero[2] = {hexWords[4][0], '\0'};
                        types::Byte byte =
                            ::strtoul(valueAtIndexZero, nullptr, 16);

                        if ((byte & constants::terminatingBit) != 0x00 &&
                            (hexWords[0][0] == 'B' && hexWords[0][1] == 'D'))
                        {
                            // if terminating bit is set and response
                            // code is for BMC i.e "BD". Send it
                            // directly to display.
                            utils::sendCurrDisplayToPanel(
                                hexWords.at(4), std::string{}, transport);
                        }
                        return;
                    }
                    std::cerr << "Event Id length is invalid" << std::endl;
                    return;
                }
                else
                {
                    std::cerr
                        << "Error fetching value of EventID. Ignoring the PEL."
                        << std::endl;
                }
            }
        }
        else
        {
            std::cerr << "Error fetching value of Severity. Ignoring the PEL"
                      << std::endl;
        }
    }
}

void PELListener::setPelRelatedFunctionState(
    const sdbusplus::message::object_path& pelObjPath)
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

    const auto res = utils::readBusProperty<std::variant<std::string>>(
        "xyz.openbmc_project.Logging", pelObjPath,
        "xyz.openbmc_project.Logging.Entry", "Resolution");

    if (const auto& resolution = std::get_if<std::string>(&res))
    {
        if (!(*resolution).empty())
        {
            std::vector<std::string> callOutList;
            boost::split(callOutList, *resolution, boost::is_any_of("\n"));

            // Need to show max 6 callout src.
            auto size = std::min(callOutList.size(), static_cast<size_t>(6));

            // default list: 14 to 19 are the functions to display
            // callout SRCs.
            constexpr std::array<types::FunctionNumber, 6> callOutSRCFunctions{
                14, 15, 16, 17, 18, 19};

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
            std::cout << "Resolution is empty for PEL = "
                      << (std::string)pelObjPath << std::endl;
        }
    }
    else
    {
        std::cerr << "Error fetching value of resolution for PEL = "
                  << (std::string)pelObjPath << std::endl;
    }

    if (list.size() > 0)
    {
        stateManager->enableFunctonality(list);
    }
}

void PELListener::getListOfExistingPels()
{
    auto listOfSortedPels = utils::geListOfPELsAndSRCs();

    // Implies there are PELs logged in the system with desired severity
    // before panel came up.
    if (!listOfSortedPels.empty())
    {
        // store the last pel details. Required to compare and disable functions
        // 11 to 19 in case this PEL gets deleted.
        lastPelObjPath = std::get<0>(listOfSortedPels[0]);
        executor->storeLastPelEventId(std::get<1>(listOfSortedPels[0]));

        // enable or disable functions based on latest PEL logged.
        setPelRelatedFunctionState(lastPelObjPath);
    }
}

void PELListener::listenPelEvents()
{
    static auto sigMatch = std::make_unique<sdbusplus::bus::match::match>(
        *conn,
        sdbusplus::bus::match::rules::interfacesAdded(
            "/xyz/openbmc_project/logging"),
        [this](sdbusplus::message::message& msg) { PELEventCallBack(msg); });

    static auto infRemovedSigMatch =
        std::make_unique<sdbusplus::bus::match::match>(
            *conn,
            sdbusplus::bus::match::rules::interfacesRemoved(
                "/xyz/openbmc_project/logging"),
            [this](sdbusplus::message::message& msg) {
                PELDeleteEventCallBack(msg);
            });

    getListOfExistingPels();
}

void PELListener::PELDeleteEventCallBack(sdbusplus::message::message& msg)
{
    sdbusplus::message::object_path objPath;
    std::vector<std::string> interface;
    msg.read(objPath, interface);

    for (const auto& element : interface)
    {
        if (element == "xyz.openbmc_project.Object.Delete")
        {
            if (objPath == lastPelObjPath)
            {
                // We need to disable function 11 to 19 as the last PEL of
                // required severity has been deleted.
                stateManager->disableFunctonality(
                    {11, 12, 13, 14, 15, 16, 18, 18, 19});
                lastPelObjPath.clear();
            }
        }
    }
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

    std::string interface;
    std::map<std::string, std::variant<PostCode>> propertyMap;

    msg.read(interface, propertyMap);

    // property we are looking for.
    const auto it = propertyMap.find("Value");
    if (it != propertyMap.end())
    {
        if (auto postCodeData = std::get_if<PostCode>(&(it->second)))
        {
            auto src = std::get<0>(*postCodeData);

            // clear display if progress code ascii equals to
            // "00000000"
            if (src == constants::clearDisplayProgressCode)
            {
                utils::sendCurrDisplayToPanel(std::string{}, std::string{},
                                              transport);
                // default the display by executing function 01.
                executor->executeFunction(1, types::FunctionalityList{});
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

            // Read the hexwords sent down by Phyp. If the hexwords are present
            // we need to store the SRC to show in function 11 and Hexwords to
            // show in function 12 and 13.
            std::vector<types::Byte> hexWordArray = std::get<1>(*postCodeData);

            // Its a fixed size array of length 72.
            if (hexWordArray.empty() || hexWordArray.size() < 72)
            {
                std::cerr << "Error reading postcode byte array" << std::endl;
                return;
            }

            // To detect if there is a need to save SRCs and hexwords in func 11
            // to 13, check for array data filled with space.
            if (hexWordArray.at(0) != 0x20)
            {
                // store the SRC.
                std::string hexWordsWithSRC =
                    std::string(byteArray.begin(), byteArray.end());

                // 4th byte will return number of valid hex words.
                types::Byte validHexWords = hexWordArray.at(3);

                // Ignoring the first 8 bytes from the array as those are some
                // header related data not HEX words. Hex words are represented
                // as 4 bytes so read 4*validHexWords bytes to read all the
                // hexwords.
                std::ostringstream convert;
                for (size_t arrayLoop = 8; arrayLoop <= (4 * validHexWords);)
                {
                    // clear any previous data.
                    convert.str("");

                    hexWordsWithSRC += " ";

                    // From this offet read 4 bytes, convert them to HEX and
                    // then append to final string of hexwords and SRC.
                    for (size_t hexWordLoop = 0; hexWordLoop < 4; ++hexWordLoop)
                    {
                        convert
                            << std::setfill('0') << std::setw(2) << std::hex
                            << static_cast<int>(hexWordArray.at(arrayLoop++));

                        hexWordsWithSRC += convert.str();
                    }
                }
                executor->storeSRCAndHexwords(hexWordsWithSRC);
            }
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
        powerPolicy = "xyz.openbmc_project.Control.Power.RestorePolicy."
                      "Policy.Restore";
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
    if (powerPolicy == "xyz.openbmc_project.Control.Power.RestorePolicy.Policy."
                       "AlwaysOff" &&
        rebootPolicy == false)
    {
        std::cout << "System operating mode set to Manual" << std::endl;
        stateManager->setSystemOperatingMode("Manual");
    }
    else
    {
        // if any of the condition fails set mode to normal
        std::cout << "System operating mode set to Normal as power policy ="
                  << powerPolicy << " and reboot policy = " << rebootPolicy
                  << std::endl;
        stateManager->setSystemOperatingMode("Normal");
    }
}
} // namespace panel
