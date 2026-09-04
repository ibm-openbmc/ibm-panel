#include "const.hpp"
#include "state_manager.hpp"
#include "transport.hpp"
#include "utils.hpp"

#include <algorithm>
#include <boost/asio/io_context.hpp>
#include <memory>
#include <phosphor-logging/lg2.hpp>
#include <sdbusplus/asio/connection.hpp>
#include <sdbusplus/asio/object_server.hpp>

/**
 * @brief Determine the panel role based on the system IM value.
 *
 * Checks the given IM value against the list of known redundant-BMC system IMs.
 *
 * For redundant-BMC systems, Unknown is considered the default role.
 * For single BMC systems, all role bits (Active, Passive, and Unknown)
 * are set in the default role mask to ignore role-based function enablement
 * during application startup.
 *
 * @param[in] im - IM value.
 *
 * @return Default role value for redundant-BMC systems, or the role mask for
 * single BMC systems.
 *
 * @throw exception
 */
panel::types::FunctionMask getDefaultPanelRole(const std::string& im)
{
    if (std::ranges::contains(panel::constants::redundantBmcSystemImList, im))
    {
        lg2::info(
            "Redundant BMC system detected (IM={IM}); assigning Unknow role",
            "IM", im);
        return panel::constants::roleUnknown;
    }

    lg2::info(
        "Single BMC system detected (IM={IM}); assigning role bits as high",
        "IM", im);
    return panel::constants::roleMask;
}

/**
 * @brief Initialise the panel subsystem.
 *
 * Reads the system IM, determines the panel role, then creates a Transport
 * and PanelStateManager instance.
 */
void initPanel() noexcept
{
    try
    {
        // Read IM and determine role mask before creating the state manager.
        const std::string imValue = panel::utils::getSystemIM();
        if (imValue.empty())
        {
            lg2::error(
                "Failed to read IM value; skipping panel initialisation.");
            return;
        }

        const panel::types::FunctionMask defaultRole =
            getDefaultPanelRole(imValue);

        // TODO: Pass real devPath, devAddr and fruPath once available.
        auto transport = std::make_shared<panel::Transport>();

        // TODO: Update PanelStateManager to accept an Executor once available.
        auto stateManager =
            std::make_shared<panel::StateManager>(transport, defaultRole);
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
