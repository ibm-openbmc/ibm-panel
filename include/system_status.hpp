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
     * @param[in] connection - Dbus connection
     * @param[in,out] stateManager - StateManager object
     * @param[in,out] transport - Transport object used to communicate with the
     * LCD panel.
     * @param[in] configJson - System config JSON object
     *
     * @throw std::runtime_error
     */
    SystemStatus(std::shared_ptr<sdbusplus::asio::connection>& connection,
                 std::shared_ptr<StateManager>& stateManager,
                 std::shared_ptr<Transport>& transport,
                 const nlohmann::json& configJson) :
        connection(connection), stateManager(stateManager),
        transport(transport), sysConfigJson(configJson)
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
    /**
     * @brief Create a NameOwnerChanged listener for the given D-Bus service
     *
     * Registers a signal match on the D-Bus that fires whenever the named
     * service starts or stops, and stores the created match object in the
     * class member. On callback, each property listed for this service in
     * the config JSON will be read from D-Bus and the StateManager API will be
     * invoked to update the system status, only if a property read succeeds.
     *
     * @param[in] service - D-Bus service name to watch.
     *
     * @throw std::runtime_error
     */
    void createNameOwnerChangedListener(const std::string& service);

    /**
     * @brief Check whether a D-Bus service is currently running
     *
     * @param[in] service - D-Bus service name to check.
     *
     * @return true if the service is running; false otherwise.
     */
    bool isServiceRunning(const std::string& service) noexcept;

    /**
     * @brief Create a property changed listener
     *
     * Registers a signal match on the bus that fires whenever any property on
     * @p interface at @p objectPath changes value, and stores the created
     * match object in the class member. The callback reads the updated
     * property value and calls the StateManager API to update the system
     * status.
     *
     * @param[in] objectPath - D-Bus object path to watch.
     * @param[in] interface - D-Bus interface name to watch.
     *
     * @throw std::runtime_error
     */
    void createPropertiesChangedListener(const std::string& objectPath,
                                         const std::string& interface);

    /** @brief D-Bus connection */
    std::shared_ptr<sdbusplus::asio::connection> connection;

    /** @brief StateManager object */
    std::shared_ptr<StateManager> stateManager;

    /** @brief Transport object */
    std::shared_ptr<Transport> transport;

    /** @brief System config JSON */
    const nlohmann::json& sysConfigJson;
};
} // namespace panel
