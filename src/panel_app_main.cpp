#include "bus_monitor.hpp"
#include "button_handler.hpp"
#include "const.hpp"
#include "panel_app.hpp"
#include "utils.hpp"

#include <exception>
#include <iostream>
#include <sdbusplus/asio/connection.hpp>
#include <sdbusplus/asio/object_server.hpp>

panel::types::PanelDataMap baseDataMap = {
    {panel::constants::rain2s2uIM,
     {panel::constants::baseDevPath, panel::constants::devAddr,
      panel::constants::rainBaseDbusObj}},
    {panel::constants::rain2s4uIM,
     {panel::constants::baseDevPath, panel::constants::devAddr,
      panel::constants::rainBaseDbusObj}},
    {panel::constants::rain1s4uIM,
     {panel::constants::baseDevPath, panel::constants::devAddr,
      panel::constants::rainBaseDbusObj}},
    {panel::constants::everestIM,
     {panel::constants::baseDevPath, panel::constants::devAddr,
      panel::constants::everBaseDbusObj}}};

panel::types::PanelDataMap lcdDataMap = {
    {panel::constants::rain2s2uIM,
     {panel::constants::rainLcdDevPath, panel::constants::devAddr,
      panel::constants::rainLcdDbusObj}},
    {panel::constants::rain2s4uIM,
     {panel::constants::rainLcdDevPath, panel::constants::devAddr,
      panel::constants::rainLcdDbusObj}},
    {panel::constants::rain1s4uIM,
     {panel::constants::rainLcdDevPath, panel::constants::devAddr,
      panel::constants::rainLcdDbusObj}},
    {panel::constants::everestIM,
     {panel::constants::everLcdDevPath, panel::constants::devAddr,
      panel::constants::everLcdDbusObj}}};

void progressCodeCallBack(sdbusplus::message::message& msg)
{
    using PostCode = std::tuple<uint64_t, std::vector<uint8_t>>;

    std::string interface{};
    std::string propVal{};
    std::map<std::string, std::variant<PostCode>> propertyMap;

    msg.read(interface, propertyMap);

    // property we are looking for.
    const auto it = propertyMap.find("Value");
    if (it != propertyMap.end())
    {
        if (auto postCodeData = std::get_if<PostCode>(&(it->second)))
        {
            uint64_t src = std::get<0>(*postCodeData);
            std::ostringstream os;
            os << src;
            propVal = os.str();

            std::cout << "Progress code SRC value = " << propVal << std::endl;
        }
        else
        {
            std::cerr << "Progress code Data error" << std::endl;
        }
        // TODO:
        // send the data to display?
    }
}

std::string getIM()
{
    auto im = panel::utils::readBusProperty<std::variant<panel::types::Binary>>(
        panel::constants::inventoryManagerIntf, panel::constants::systemDbusObj,
        panel::constants::imInterface, panel::constants::imKeyword);
    if (auto imVector = std::get_if<panel::types::Binary>(&im))
    {
        return panel::utils::binaryToHexString(*imVector);
    }
    else
    {
        std::cerr << "\n Failed querying IM property from dbus" << std::endl;
    }
    return "";
}

std::string getInputDevicePath(const std::string& imValue)
{
    if (imValue == panel::constants::rain2s2uIM ||
        imValue == panel::constants::rain2s4uIM ||
        imValue == panel::constants::rain1s4uIM)
    {
        return "/dev/input/by-path/platform-1e78a400.i2c-bus-event-joystick";
    }
    else if (imValue == panel::constants::everestIM)
    {
        // TODO: This path needs to be modified with Everest path.
        return "/dev/input/by-path/platform-1e78a400.i2c-bus-event-joystick";
    }

    // default to tacoma
    return "/dev/input/by-path/platform-1e78a080.i2c-bus-event-joystick";
}

bool getPresentProperty(const std::string& imValue)
{
    auto present = panel::utils::readBusProperty<std::variant<bool>>(
        panel::constants::inventoryManagerIntf,
        std::get<2>((lcdDataMap.find(imValue))->second),
        panel::constants::itemInterface, "Present");
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

int main(int, char**)
{
    try
    {
        auto io = std::make_shared<boost::asio::io_context>();
        auto conn = std::make_shared<sdbusplus::asio::connection>(*io);
        conn->request_name("com.ibm.PanelApp");

        auto server = sdbusplus::asio::object_server(conn);

        std::shared_ptr<sdbusplus::asio::dbus_interface> iface =
            server.add_interface("/com/ibm/panel_app", "com.ibm.panel");

        iface->register_method("Display", display);

        // register to progress code property change.
        auto progressCodeSignal =
            std::make_unique<sdbusplus::bus::match::match>(
                *conn,
                sdbusplus::bus::match::rules::propertiesChanged(
                    "/xyz/openbmc_project/state/boot/raw0",
                    "xyz.openbmc_project.State.Boot.Raw"),
                progressCodeCallBack);

        const std::string imValue = getIM();

        // create transport lcd object
        auto lcdPanelObj = std::make_shared<panel::Transport>(
            std::get<0>((lcdDataMap.find(imValue))->second),
            std::get<1>((lcdDataMap.find(imValue))->second),
            panel::types::PanelType::LCD);

        // create transport base object
        auto basePanelObj = std::make_shared<panel::Transport>(
            std::get<0>((baseDataMap.find(imValue))->second),
            std::get<1>((baseDataMap.find(imValue))->second),
            panel::types::PanelType::BASE);
        basePanelObj->setTransportKey(true);

        // Listen to lcd panel presence always for both rainier and everest
        panel::PanelPresence presenceObj(
            std::get<2>((lcdDataMap.find(imValue))->second), conn, lcdPanelObj);
        presenceObj.listenPanelPresence();

        // Race condition can happen when the panel is removed exactly at the
        // time after setting the transport key(to true - for the first time)
        // and before firing the match signal. Here after removing the panel,
        // "Properties.Changed" signal will wait for a property change from
        // false to true; but the transport key is true(unchanged). To maintain
        // data accuracy get the "Present" property from dbus and set the
        // transport key again.

        lcdPanelObj->setTransportKey(getPresentProperty(imValue));

        // create state manager object
        auto stateManagerObj =
            std::make_shared<panel::state::manager::PanelStateManager>(
                lcdPanelObj);

        panel::ButtonHandler btnHandler(getInputDevicePath(imValue), io,
                                        lcdPanelObj, stateManagerObj);

        io->run();
    }
    catch (const std::exception& e)
    {
        std::cerr << e.what();
        throw;
    }
    // Create the D-Bus invoker class
    // Create D-Bus signal handler
}
