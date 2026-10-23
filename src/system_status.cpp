#include "system_status.hpp"

#include "utils.hpp"

#include <phosphor-logging/lg2.hpp>

namespace panel
{
void SystemStatus::initialize() noexcept
{
    try
    {
        if (!sysConfigJson.contains("listenOnProperties"))
        {
            lg2::error("'listenOnProperties' tag not found");
            return;
        }

        // Tracks whether at least one property was successfully read.
        // The StateManager API is called once after all entries are processed,
        // only if this is true.
        bool readAnyProperty = false;

        const auto& listenOnProps = sysConfigJson["listenOnProperties"];

        for (const auto& [service, objectList] : listenOnProps.items())
        {
            // Create NameOwnerChanged match for this service.
            createNameOwnerChangedListener(service);

            // Check if the service is already running.
            const bool serviceRunning = isServiceRunning(service);

            for (const auto& objEntry : objectList)
            {
                for (const auto& [objectPath, intfList] : objEntry.items())
                {
                    for (const auto& intfEntry : intfList)
                    {
                        const std::string interface =
                            intfEntry["interfaceName"].get<std::string>();

                        // Create PropertiesChanged match for each
                        //     object path + interface pair.
                        createPropertiesChangedListener(objectPath, interface);

                        // If service is already running, read property
                        //     values and update the state structure.
                        if (serviceRunning)
                        {
                            readAnyProperty |= readAndUpdatePropertyValue(
                                service, objectPath, interface,
                                intfEntry["properties"]);
                        }
                    }
                }
            }
        }

        if (readAnyProperty)
        {
            // ToDo: Call StateManager API with the populated status structure.
        }
    }
    catch (const std::exception& ex)
    {
        lg2::error("Failed to initialize SystemStatus, reason: {ERROR}",
                   "ERROR", ex);
        // ToDo: log a PEL
    }
}

void SystemStatus::createNameOwnerChangedListener(const std::string& service)
{
    // ToDo: Register a NameOwnerChanged signal match for this service and
    // store the created match object in the class member. On callback, each
    // property listed for this service in the config JSON will be read and
    // the StateManager API will be invoked to update the system status, only
    // if at least one property read succeeds.
    lg2::info("registered NameOwnerChanged watcher for service {SVC}", "SVC",
              service);
}

bool SystemStatus::isServiceRunning(
    [[maybe_unused]] const std::string& service) noexcept
{
    // ToDo: Query is given Dbus service running and return the result
    return false;
}

void SystemStatus::createPropertiesChangedListener(
    const std::string& objectPath, const std::string& interface)
{
    // ToDo: Register a PropertiesChanged signal match for objectPath /
    // interface and store the created match object in the class member. The
    // callback should read the updated property value and call the StateManager
    // API to update the system status.
    lg2::info("registered PropertiesChanged watcher [{PATH}] [{INTF}]", "PATH",
              objectPath, "INTF", interface);
}

bool SystemStatus::readAndUpdatePropertyValue(
    [[maybe_unused]] const std::string& service,
    [[maybe_unused]] const std::string& objectPath,
    [[maybe_unused]] const std::string& interface,
    [[maybe_unused]] const auto& properties) noexcept
{
    // ToDo: For each property in properties:
    //   1. Call utils::readDbusProperty(service, objectPath, interface,
    //      property) to get the current value from D-Bus.
    //   2. On success, store the returned value in the dedicated field of the
    //      system status structure (structure to be defined; each property has
    //      its own field) and set a local flag to true.
    // Return the flag — true if at least one property was read & stored
    // successfully.
    return false;
}
} // namespace panel
