#include "const.hpp"
#include "panel_state_manager.hpp"
#include "transport.hpp"

#include <boost/asio/io_context.hpp>
#include <memory>
#include <phosphor-logging/lg2.hpp>
#include <sdbusplus/asio/connection.hpp>
#include <sdbusplus/asio/object_server.hpp>

/**
 * @brief Initialise the panel subsystem.
 *
 * Creates a Transport instance and other panel-level objects required at
 * start-up.
 */
void initPanel() noexcept
{
    try
    {
        // TODO: Pass real devPath, devAddr and fruPath once available.
        auto transport = std::make_shared<panel::Transport>();

        // TODO: Update PanelStateManager to accept an Executor once available.
        auto stateManager =
            std::make_shared<panel::state::manager::PanelStateManager>(
                transport);
    }
    catch (const std::exception& ex)
    {
        lg2::error("Failed to initialise the panel, reason: {ERROR}", "ERROR",
                   ex);
        // TODO: log a critical PEL
    }
}

int main()
{
    try
    {
        auto io = std::make_shared<boost::asio::io_context>();
        auto conn = std::make_shared<sdbusplus::asio::connection>(*io);

        // Request DBus name
        conn->request_name(panel::constants::panelService);

        // Create object server
        sdbusplus::asio::object_server server(conn);

        // Add the interface
        std::shared_ptr<sdbusplus::asio::dbus_interface> iface =
            server.add_interface(panel::constants::panelObjectPath,
                                 panel::constants::panelInterface);

        initPanel();

        iface->initialize();

        // Run the event loop
        io->run();
    }
    catch (const std::exception& ex)
    {
        lg2::error("Panel application terminated due to exception: {ERROR}",
                   "ERROR", ex.what());
        return -1;
    }

    return 0;
}
