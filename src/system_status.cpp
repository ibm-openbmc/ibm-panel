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

        // Read & store initial system status properties value from D-Bus
        types::SystemStatusInfo statusInfo;

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

                        if (serviceRunning)
                        {
                            readAnyProperty |= readAndStorePropertyValue(
                                service, objectPath, interface,
                                intfEntry["properties"], statusInfo);
                        }
                    }
                }
            }
        }

        if (readAnyProperty)
        {
            stateManager->applySystemStatusInfo(statusInfo);
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
    nameOwnerChangedMatches.emplace_back(
        *connection, sdbusplus::bus::match::rules::nameOwnerChanged(service),
        [this, service](sdbusplus::message_t& msg) {
            nameOwnerChangedCallback(msg, service);
        });

    lg2::info("registered NameOwnerChanged watcher for service {SVC}", "SVC",
              service);
}

void SystemStatus::nameOwnerChangedCallback(sdbusplus::message_t& msg,
                                            const std::string& service) noexcept
{
    try
    {
        std::string name;
        std::string oldOwner;
        std::string newOwner;
        msg.read(name, oldOwner, newOwner);

        // An empty new owner means the service went down; nothing to read.
        if (newOwner.empty())
        {
            return;
        }

        for (const auto& objEntry :
             sysConfigJson["listenOnProperties"][service])
        {
            for (const auto& [objectPath, intfList] : objEntry.items())
            {
                for (const auto& intfEntry : intfList)
                {
                    const std::string interface =
                        intfEntry["interfaceName"].get<std::string>();

                    for (const auto& property : intfEntry["properties"])
                    {
                        const auto propertyName = property.get<std::string>();
                        const auto readRes = utils::readDbusProperty(
                            service, objectPath, interface, propertyName);
                        if (!readRes)
                        {
                            continue;
                        }

                        dispatchProperty(service, objectPath, interface,
                                         propertyName, readRes.value());
                    }
                }
            }
        }
    }
    catch (const std::exception& ex)
    {
        lg2::error("Failed to handle service start for {SVC}, reason: {ERROR}",
                   "SVC", service, "ERROR", ex);
    }
}

void SystemStatus::dispatchProperty(
    [[maybe_unused]] const std::string& service,
    [[maybe_unused]] const std::string& objectPath,
    [[maybe_unused]] const std::string& interface,
    [[maybe_unused]] const std::string& property,
    [[maybe_unused]] const types::DbusVariantType& value) noexcept
{
    try
    {
        // ToDo: dispatch property to dedicated StateManager API based on the
        // recieved input details.
    }
    catch (const std::exception& ex)
    {
        lg2::error("Handler failed for {INTF} {PROP}, reason: {ERROR}", "INTF",
                   interface, "PROP", property, "ERROR", ex);
    }
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

bool SystemStatus::readAndStorePropertyValue(
    [[maybe_unused]] const std::string& service,
    [[maybe_unused]] const std::string& objectPath,
    [[maybe_unused]] const std::string& interface,
    [[maybe_unused]] const auto& properties,
    [[maybe_unused]] types::SystemStatusInfo& statusInfo) noexcept
{
    // ToDo: For each property in properties:
    //   1. Call utils::readDbusProperty(service, objectPath, interface,
    //      property) to get the current value from D-Bus.
    //   2. On success, store the returned value in the dedicated field of
    //      SystemStatusInfo (each property has its own field) and set a local
    //      flag to true.
    // Return the flag — true if at least one property was read & stored
    // successfully.
    return false;
}
} // namespace panel
