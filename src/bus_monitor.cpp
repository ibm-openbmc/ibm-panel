#include "bus_monitor.hpp"

#include "const.hpp"

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
} // namespace panel