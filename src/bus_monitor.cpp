#include "bus_monitor.hpp"

#include "const.hpp"
#include "utils.hpp"

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

        const auto propItr = propMap.find("Severity");
        if (propItr != propMap.end())
        {
            const auto severity = std::get_if<std::string>(&propItr->second);

            // TODO: need to check which all severty needs to be taken care
            // of
            if (severity != nullptr &&
                *severity !=
                    "xyz.openbmc_project.Logging.Entry.Level.Informational")
            {
                if (!functionStateEnabled)
                {
                    functionStateEnabled = true;
                    types::FunctionalityList list{11, 12, 13};
                    stateManager->enableFunctonality(list);
                }

                // save the last predictive/unrecoverable error ID.
                // TODO: how many we need to save. Function name to change
                // accordingly.
                executor->lastPELId(objPath);
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
        }
        else
        {
            std::cerr << "Progress code Data error" << std::endl;
        }
    }
}
} // namespace panel