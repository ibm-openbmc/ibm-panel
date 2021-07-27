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
} // namespace panel