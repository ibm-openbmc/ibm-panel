#include "bus_handler.hpp"
#include "bus_monitor.hpp"
#include "button_handler.hpp"
#include "const.hpp"
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
        return "/dev/input/by-path/platform-1e78a780.i2c-bus-event-joystick";
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

void getLcdDeviceData(std::string& lcdDevPath, uint8_t& lcdDevAddr,
                      std::string& lcdObjPath, const std::string& imValue)
{
    if (lcdDataMap.find(imValue) ==
        lcdDataMap.end()) // assume the system is tacoma
    {
        lcdDevPath = panel::constants::tacomaLcdDevPath;
        lcdDevAddr = panel::constants::devAddr;
    }
    else
    {
        lcdDevPath = std::get<0>((lcdDataMap.find(imValue))->second);
        lcdDevAddr = std::get<1>((lcdDataMap.find(imValue))->second);
        lcdObjPath = std::get<2>((lcdDataMap.find(imValue))->second);
    }
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

        const std::string imValue = getIM();

        std::string lcdDevPath{}, lcdObjPath{};
        uint8_t lcdDevAddr;
        getLcdDeviceData(lcdDevPath, lcdDevAddr, lcdObjPath, imValue);

        // create transport lcd object
        auto lcdPanel = std::make_shared<panel::Transport>(
            lcdDevPath, lcdDevAddr, panel::types::PanelType::LCD);

        // create transport base object
        std::shared_ptr<panel::Transport> basePanel;
        if (baseDataMap.find(imValue) != baseDataMap.end())
        {
            basePanel = std::make_shared<panel::Transport>(
                std::get<0>((baseDataMap.find(imValue))->second),
                std::get<1>((baseDataMap.find(imValue))->second),
                panel::types::PanelType::BASE);
            basePanel->setTransportKey(true);
        }

        // Listen to lcd panel presence always for both rainier and everest
        std::unique_ptr<panel::PanelPresence> presence;

        if (lcdDataMap.find(imValue) != lcdDataMap.end())
        {
            presence = std::make_unique<panel::PanelPresence>(lcdObjPath, conn,
                                                              lcdPanel);
            presence->listenPanelPresence();

            /** Race condition can happen when the panel is removed exactly at
             * the time after setting the transport key(to true - for the first
             * time) and before firing the match signal. After removing the
             * panel, "Properties.Changed" signal will wait for a property
             * change from false to true; but the transport key is still
             * true(unchanged). To maintain data accuracy get the "Present"
             * property from dbus and set the transport key again.*/
            lcdPanel->setTransportKey(getPresentProperty(imValue));
        }
        else
        {
            // set transport key to true for test system(tacoma).
            lcdPanel->setTransportKey(true);
        }

        // create executor class
        auto executor = std::make_shared<panel::Executor>(lcdPanel);

        // create state manager object
        auto stateManager =
            std::make_shared<panel::state::manager::PanelStateManager>(
                lcdPanel, executor);

        // TODO: via https://github.com/ibm-openbmc/ibm-panel/issues/21
        // Remove this try catch around the button handler once Everest device
        // tree changes are ready.
        std::unique_ptr<panel::ButtonHandler> btnHandler;
        try
        {
            btnHandler = std::make_unique<panel::ButtonHandler>(
                getInputDevicePath(imValue), io, lcdPanel, stateManager);
        }
        catch (const std::runtime_error& e)
        {
            std::cerr << e.what() << std::endl;
            std::cerr << "Could not initialize button handler, panel buttons "
                         "will not work!"
                      << std::endl;
        }


        panel::PELListener pelEvent(conn, stateManager, executor);
        pelEvent.listenPelEvents();

        // register property change call back for progress code.
        panel::BootProgressCode progressCode(lcdPanel, conn, executor);
        progressCode.listenProgressCode();

        panel::BusHandler busHandle(lcdPanel, iface);

        iface->initialize();

        panel::SystemStatus systemStatus(conn, stateManager);

        io->run();
    }
    catch (const std::exception& e)
    {
        std::cerr << e.what();
        std::cerr << "Panel app exiting..." << std::endl;
        return 0;
        // TODO: Need to rethrow here so that systemd can mark the service a
        // failure. We will do that once Everest hardware is ready.
        // https://github.com/ibm-openbmc/ibm-panel/issues/21
    }
    return 0;
}
