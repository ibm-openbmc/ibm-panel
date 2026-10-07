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

namespace panel
{
/**
 * @brief Aggregates all long-lived panel subsystem objects.
 *
 * Holds the objects that need to survive for the duration of the event loop.
 */
struct PanelContext
{
    std::shared_ptr<panel::Transport> transport;
    std::shared_ptr<panel::StateManager> stateManager;
    nlohmann::json sysConfigJson;
};

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
types::RoleType getDefaultPanelRole(const std::string& im)
{
    if (std::ranges::contains(constants::redundantBmcSystemImList, im))
    {
        lg2::info(
            "Redundant BMC system detected (IM={IM}); assigning Unknow role",
            "IM", im);
        return constants::roleUnknown;
    }

    lg2::info(
        "Single BMC system detected (IM={IM}); assigning role bits as high",
        "IM", im);
    return constants::roleMask;
}

/**
 * @brief Sets transport key for LCD panel for multi-BMC system.
 *
 * This API enables the transport key for redundant-BMC systems. It reads the
 * BMC position from D-Bus; the position is used to determine panel control
 * ownership via the `Patch Panel` control bits.
 *
 * Enabling the transport key is based on the following criteria:
 *     # Panel device should be present.
 *     # Read the panel control bits from the `Patch Panel` to determine
 *       who owns the panel control.
 *     # If current BMC owns the control, enable the transport key for the
 *       LCD device.
 * In case of any error, log a PEL.
 *
 * Note: Setting transport key to true allows the Transport class to access the
 * i2c bus.
 *
 * @param[in] transport - The transport object.
 */
void setTransportKeyForMultiBmcSystem(
    [[maybe_unused]] const auto& transport) noexcept
{
    std::string errMessage{};
    try
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

        const auto positionRes = utils::readDbusProperty(
            constants::pimService, constants::systemInvPath,
            constants::positionInterface, constants::positionPropertyName);

        if (!positionRes)
        {
            errMessage = std::format(
                "Failed to read BMC Position from D-Bus, reason: {}",
                utils::getErrCodeMsg(positionRes.error()));
        }
        else if (const auto val = std::get_if<size_t>(&positionRes.value()))
        {
            if (*val == constants::VALUE_0)
            {
                // ToDo: set transport key
                return;
            }
        }
        else
        {
            errMessage =
                "Invalid type received while reading BMC Position from D-Bus";
        }
    }
    catch (const std::exception& ex)
    {
        errMessage = std::format("Error occurred while reading and enabling "
                                 "Panel ownership, reason: {}",
                                 ex.what());
    }

    if (!errMessage.empty())
    {
        lg2::error("{ERR}", "ERR", errMessage);
        utils::createPEL("com.ibm.Panel.Error.InternalFailure",
                         "xyz.openbmc_project.Logging.Entry.Level.Warning",
                         {{"DESCRIPTION", errMessage}});
    }
}

/**
 * @brief Initialise the panel subsystem.
 *
 * Reads the system IM, determines the panel role, and populates all
 * long-lived subsystem objects into the provided PanelContext.
 *
 * @param[out] ctx - PanelContext to populate. Members are left unchanged
 *                   on failure.
 */
void initPanel(PanelContext& ctx) noexcept
{
    try
    {
        // Read IM and determine role mask before creating the state manager.
        const auto imResult = utils::getSystemIm();
        if (!imResult)
        {
            throw std::runtime_error(
                std::format("Error occured while reading system IM value from "
                            "D-Bus, reason: {}",
                            utils::getErrCodeMsg(imResult.error())));
        }
        else if (imResult.value().empty())
        {
            throw std::runtime_error("System IM value found empty");
        }

        // Parse the panel configuration for this system.
        const std::string configPath =
            utils::getPanelConfigPath(imResult.value());

        const auto configResult = utils::getParsedJson(configPath);
        if (!configResult)
        {
            throw std::runtime_error(std::format(
                "Failed to parse panel configuration [{}], reason: {}",
                configPath, utils::getErrCodeMsg(configResult.error())));
        }

        ctx.sysConfigJson = configResult.value();

        // TODO: Move role fetching to SystemStatus once the class is
        // implemented.
        const types::RoleType defaultRole =
            getDefaultPanelRole(imResult.value());

        // TODO: Pass real devPath, devAddr and fruPath once available.
        ctx.transport = std::make_shared<panel::Transport>();

        // For redundant-BMC systems, enable the transport key only if the
        // current BMC owns the panel control.
        if (std::ranges::contains(constants::redundantBmcSystemImList,
                                  imResult.value()))
        {
            setTransportKeyForMultiBmcSystem(ctx.transport);
        }
        else
        {
            // ToDo: Check the LCD Panel presence and enable the trasport key
            // based on the device presence.
        }

        // TODO: Update PanelStateManager to accept an Executor once available.
        ctx.stateManager =
            std::make_shared<panel::StateManager>(ctx.transport, defaultRole);
    }
    catch (const std::exception& ex)
    {
        lg2::error("Failed to initialise Panel, reason: {ERROR}", "ERROR", ex);

        utils::createPEL("com.ibm.Panel.Error.InternalFailure",
                         "xyz.openbmc_project.Logging.Entry.Level.Warning",
                         {{"DESCRIPTION",
                           std::format("Failed to initialise Panel, reason: {}",
                                       ex.what())}});
    }
}
} // namespace panel

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

        // Construct ctx in main() so all subsystem objects remain alive
        // for the entire event loop.
        panel::PanelContext ctx;
        panel::initPanel(ctx);

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
