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
      panel::constants::everBaseDbusObj}},
    {panel::constants::balconesIM,
     {panel::constants::balconesBaseDevPath, panel::constants::devAddr,
      panel::constants::balconesBaseDbusObj}},
    {panel::constants::blueridge2s2uIM,
     {panel::constants::baseDevPath, panel::constants::devAddr,
      panel::constants::blueridgeBaseDbusObj}},
    {panel::constants::blueridge2s4uIM,
     {panel::constants::baseDevPath, panel::constants::devAddr,
      panel::constants::blueridgeBaseDbusObj}},
    {panel::constants::blueridge1s4uIM,
     {panel::constants::baseDevPath, panel::constants::devAddr,
      panel::constants::blueridgeBaseDbusObj}},
    {panel::constants::fujiIM,
     {panel::constants::baseDevPath, panel::constants::devAddr,
      panel::constants::fujiBaseDbusObj}},
};

std::string getInputDevicePath(const std::string& imValue)
{
    if (imValue == panel::constants::rain2s2uIM ||
        imValue == panel::constants::rain2s4uIM ||
        imValue == panel::constants::rain1s4uIM ||
        imValue == panel::constants::balconesIM ||
        imValue == panel::constants::blueridge2s2uIM ||
        imValue == panel::constants::blueridge2s4uIM ||
        imValue == panel::constants::blueridge1s4uIM)
    {
        return "/dev/input/by-path/platform-1e78a400.i2c-event-joystick";
    }
    else if (imValue == panel::constants::everestIM ||
             imValue == panel::constants::fujiIM)
    {
        return "/dev/input/by-path/platform-1e78a780.i2c-event-joystick";
    }

    // default to tacoma
    return "/dev/input/by-path/platform-1e78a080.i2c-bus-event-joystick";
}

void getLcdDeviceData(std::string& lcdDevPath, uint8_t& lcdDevAddr,
                      std::string& lcdObjPath, const std::string& imValue)
{
    if (panel::utils::lcdDataMap.find(imValue) ==
        panel::utils::lcdDataMap.end()) // assume the system is tacoma
    {
        lcdDevPath = panel::constants::tacomaLcdDevPath;
        lcdDevAddr = panel::constants::devAddr;
    }
    else
    {
        lcdDevPath =
            std::get<0>((panel::utils::lcdDataMap.find(imValue))->second);
        lcdDevAddr =
            std::get<1>((panel::utils::lcdDataMap.find(imValue))->second);
        lcdObjPath =
            std::get<2>((panel::utils::lcdDataMap.find(imValue))->second);
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

        const std::string imValue = panel::utils::getSystemIM();

        std::string lcdDevPath{}, lcdObjPath{};
        uint8_t lcdDevAddr;
        getLcdDeviceData(lcdDevPath, lcdDevAddr, lcdObjPath, imValue);

        // create transport lcd object
        auto lcdPanel = std::make_shared<panel::Transport>(
            lcdDevPath, lcdDevAddr, panel::types::PanelType::LCD, lcdObjPath);

        // create executor class
        auto executor = std::make_shared<panel::Executor>(lcdPanel, iface, io);

        // create state manager object
        auto stateManager =
            std::make_shared<panel::state::manager::PanelStateManager>(
                lcdPanel, executor);

        // create transport base object and listen for its presence to
        // enable CM on everest.

        // NOTE:Always base transport key should be set to true prior to LCD
        // transport key getting set to true. This is to be taken care for code
        // update scenarios. Reason: If the base panel isn't up, SRC is
        // displayed on LCD panel. And there needs an external request to LCD
        // panel to change the display once the base is up. This case can get
        // avoided by following the panel order to set transport key.

        // create transport base object
        std::shared_ptr<panel::Transport> basePanel;
        std::unique_ptr<panel::PanelPresence> basePanelPresence;
        if (baseDataMap.find(imValue) != baseDataMap.end())
        {
            basePanel = std::make_shared<panel::Transport>(
                std::get<0>((baseDataMap.find(imValue))->second),
                std::get<1>((baseDataMap.find(imValue))->second),
                panel::types::PanelType::BASE,
                (panel::constants::bootFailPIC.find(imValue) !=
                 panel::constants::bootFailPIC.end())
                    ? panel::constants::bootFailPIC.find(imValue)->second
                    : std::string());

            auto& baseObjPath =
                std::get<2>((baseDataMap.find(imValue))->second);

            // if it is base panel of Everest, register for its presence to
            // enable CM.
            if (baseObjPath == panel::constants::everBaseDbusObj ||
                baseObjPath == panel::constants::fujiBaseDbusObj)
            {
                basePanelPresence = std::make_unique<panel::PanelPresence>(
                    baseObjPath, conn, basePanel, stateManager);

                basePanelPresence->listenPanelPresence();
            }

            basePanel->setTransportKey(true);
        }

        // Listen to lcd panel presence always for both rainier and everest
        std::unique_ptr<panel::PanelPresence> presence;

        if (panel::utils::lcdDataMap.find(imValue) !=
            panel::utils::lcdDataMap.end())
        {
            presence = std::make_unique<panel::PanelPresence>(
                lcdObjPath, conn, lcdPanel, stateManager);
            presence->listenPanelPresence();

            /** Race condition can happen when the panel is removed exactly at
             * the time after setting the transport key(to true - for the first
             * time) and before firing the match signal. After removing the
             * panel, "Properties.Changed" signal will wait for a property
             * change from false to true; but the transport key is still
             * true(unchanged). To maintain data accuracy get the "Present"
             * property from dbus and set the transport key again.*/
            lcdPanel->setTransportKey(
                panel::utils::getLcdPanelPresentProperty(imValue));
        }
        else
        {
            // set transport key to true for test system(tacoma).
            lcdPanel->setTransportKey(true);
        }

        // TODO: via https://github.com/ibm-openbmc/ibm-panel/issues/21
        // Remove this try catch around the button handler once Everest device
        // tree changes are ready.
        std::unique_ptr<panel::ButtonHandler> btnHandler;
        try
        {
            btnHandler = std::make_unique<panel::ButtonHandler>(
                getInputDevicePath(imValue), io, lcdPanel, stateManager,
                lcdDevPath);
        }
        catch (const std::runtime_error& e)
        {
            std::cerr << e.what() << std::endl;
            std::cerr << "Could not initialize button handler, panel buttons "
                         "will not work!"
                      << std::endl;
        }

        panel::PELListener pelEvent(conn, stateManager, executor, lcdPanel);
        pelEvent.listenPelEvents();

        // register property change call back for progress code.
        panel::BootProgressCode progressCode(lcdPanel, conn, executor,
                                             stateManager);
        progressCode.listenProgressCode();

        panel::BusHandler busHandle(lcdPanel, iface, stateManager, executor);

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
