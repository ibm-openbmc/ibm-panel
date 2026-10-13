#pragma once

#include "state_manager.hpp"
#include "transport.hpp"
#include "types.hpp"

#include <memory>
#include <nlohmann/json.hpp>
#include <sdbusplus/asio/connection.hpp>
#include <string>

namespace panel
{

/**
 * @brief Class to track the system status
 *
 * This class implements handler to listen for system parameters to
 * know the current state and operating mode of the system.
 *
 * The set of properties to listen on is driven by the system config JSON, and
 * not hard coded in this class. Each entry in the config JSON identifies a
 * dbus service, object path, interface and property. SystemStatus
 * watches these properties and hands each value to a dedicated handler
 * function for that property. The handler is responsible for converting the
 * value and updating the system status in StateManager.
 */
class SystemStatus
{
  public:
    /** @brief Deleted special members */
    SystemStatus(const SystemStatus&) = delete;
    SystemStatus& operator=(const SystemStatus&) = delete;
    SystemStatus(const SystemStatus&&) = delete;
    SystemStatus& operator=(const SystemStatus&&) = delete;

    ~SystemStatus() = default;

    /**
     * @brief Parameterized constructor
     *
     * @param[in] conn - Dbus connection
     * @param[in,out] stateManager - StateManager object
     * @param[in,out] transport - Transport object used to communicate with the
     * LCD panel.
     * @param[in] configJson - System config JSON object
     */
    SystemStatus(
        [[maybe_unused]] std::shared_ptr<sdbusplus::asio::connection>& conn,
        [[maybe_unused]] std::shared_ptr<StateManager>& stateManager,
        [[maybe_unused]] std::shared_ptr<Transport>& transport,
        const nlohmann::json& configJson) : sysConfigJson(configJson)
    {
        if (sysConfigJson.empty())
        {
            throw std::runtime_error("System config JSON is empty");
        }

        initialize();
    }

    /**
     * @brief Create dbus property listeners
     *
     * This API creates dbus listeners and registers callbacks for the
     * properties present on the config JSON. Calls StateManager API to update
     * the initale system status.
     *
     * Since the listeners are created by iterating over the config JSON,
     * a new dbus property can be monitored by adding it to the JSON and
     * providing a dedicated handler function for it.
     */
    void initialize() noexcept;

  private:
    const nlohmann::json& sysConfigJson;
};
} // namespace panel