#include "const.hpp"
#include "error_codes.hpp"
#include "state_manager.hpp"
#include "transport.hpp"
#include "utils.hpp"

#include <algorithm>
#include <boost/asio/io_context.hpp>
#include <format>
#include <limits>
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
panel::types::RoleType getDefaultPanelRole(const std::string& im)
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
 * @brief Check and enable the LCD panel transport key.
 *
 * Reads the BMC position property from the D-Bus. Use read position value to
 * read the panel control bits from microcontroller present in the `Patch
 * Panel`. If the current BMC owns the access to LCD panel, transport key is
 * enabled. Logs a PEL in case of any failure.
 *
 * @note Enabling the transport key requires:
 *       -# Reading BMC position from the DBus.
 *       -# Reading the Panel control bits from the Patch Panel to determine
 *          ownership of the LCD panel using read position information.
 *       -# If this BMC owns the position, enabling the transport key for the
 *          LCD device.
 *
 * @param[in] transport - The transport object.
 *
 * @return BMC position value on successful DBus read, otherwise max value in
 * case of any failure.
 */
size_t checkAndEnableLcdPanel([[maybe_unused]] const auto& transport) noexcept
{
    [[maybe_unused]] std::string errMessage;
    try
    {
        const auto positionRes = panel::utils::readDbusProperty(
            panel::constants::pimService, panel::constants::systemInvPath,
            panel::constants::positionInterface,
            panel::constants::positionPropertyName);

        if (!positionRes)
        {
            errMessage = std::format(
                "Failed to read BMC Position from D-Bus, reason: {}",
                panel::utils::getErrCodeMsg(positionRes.error()));
            lg2::error("{ERR}", "ERR", errMessage);
        }
        else if (const auto bmcPosition =
                     std::get_if<size_t>(&positionRes.value()))
        {
            /* ToDo enable transport key based on below criteria
             * 1. Check LCD panel is present, if device is present
             * 1.1. Read the Panel control bits from Patch Panel to know
             * who owns the position.
             * 1.2. If control is owned by current BMC, enable the transport key
             * for LCD device.
             *
             * Note: Until patch panel control-bit access is available to find
             * the Panel ownership, the BMC at position `0` is granted control.
             */

            if (*bmcPosition == panel::constants::VALUE_0)
            {
                // ToDo: set transport key
            }
            return *bmcPosition;
        }
        else
        {
            errMessage =
                "Invalid type received while reading BMC Position from D-Bus";
            lg2::error("{ERR}", "ERR", errMessage);
        }
    }
    catch (const std::exception& ex)
    {
        errMessage = std::format("Error occurred while reading and enabling "
                                 "Panel ownership, reason: {}",
                                 ex.what());
        lg2::error("{ERR}", "ERR", errMessage);
    }

    // ToDo: log a PEL with errMessage
    return std::numeric_limits<size_t>::max();
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
        const auto imResult = panel::utils::getSystemIm();
        if (!imResult)
        {
            throw std::runtime_error(
                std::format("Error occured while reading system IM value from "
                            "D-Bus, reason: {}",
                            panel::utils::getErrCodeMsg(imResult.error())));
        }
        else if (imResult.value().empty())
        {
            throw std::runtime_error("System IM value found empty");
        }

        // TODO: Move role fetching to SystemStatus once the class is
        // implemented.
        const panel::types::RoleType defaultRole =
            getDefaultPanelRole(imResult.value());

        // TODO: Pass real devPath, devAddr and fruPath once available.
        auto transport = std::make_shared<panel::Transport>();

        // BMC position is required during failover to request panel control
        // from the microcontroller on the patch panel for redundant-BMC
        // systems.
        [[maybe_unused]] size_t bmcPosition =
            std::numeric_limits<size_t>::max();

        // For redundant-BMC systems, enable the transport key only if current
        // BMC owns the LCD panel.
        if (std::ranges::contains(panel::constants::redundantBmcSystemImList,
                                  imResult.value()))
        {
            bmcPosition = checkAndEnableLcdPanel(transport);
        }
        else
        {
            // ToDo: Check the LCD Panel presence and enable the trasport key
            // based on the device presence.
        }

        // TODO: Update PanelStateManager to accept an Executor once available.
        auto stateManager =
            std::make_shared<panel::StateManager>(transport, defaultRole);
    }
    catch (const std::exception& ex)
    {
        lg2::error("Failed to initialise Panel, reason: {ERROR}", "ERROR", ex);

        panel::utils::createPEL(
            "com.ibm.Panel.Error.InternalFailure",
            "xyz.openbmc_project.Logging.Entry.Level.Warning",
            {{"DESCRIPTION",
              std::format("Failed to initialise Panel, reason: {}",
                          ex.what())}});
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
