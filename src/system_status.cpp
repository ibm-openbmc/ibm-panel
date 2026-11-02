#include "system_status.hpp"

#include "const.hpp"
#include "utils.hpp"

#include <algorithm>
#include <array>
#include <phosphor-logging/lg2.hpp>
#include <string_view>

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

void SystemStatus::handleBmcState(
    [[maybe_unused]] const types::DbusVariantType& value)
{
    // ToDo: Convert value and call the dedicated StateManager API.
}

void SystemStatus::handleBmcRole(
    [[maybe_unused]] const types::DbusVariantType& value)
{
    // ToDo: Convert value and call the dedicated StateManager API.
}

void SystemStatus::dispatchProperty(
    const std::string& service, const std::string& objectPath,
    const std::string& interface, const std::string& property,
    const types::DbusVariantType& value) noexcept
{
    try
    {
        /** @brief Handler function type for a D-Bus property value */
        using PropertyHandler =
            void (SystemStatus::*)(const types::DbusVariantType&);

        /** @brief Identifies a D-Bus property and its dedicated handler */
        struct PropertyHandlerEntry
        {
            std::string_view service;
            std::string_view objectPath;
            std::string_view interface;
            std::string_view property;
            PropertyHandler handler;
        };

        static constexpr std::array handlers = {
            PropertyHandlerEntry{
                constants::bmcStateServiceName, constants::bmcStateObjectPath,
                constants::bmcStateInterface, "CurrentBMCState",
                &SystemStatus::handleBmcState},
            PropertyHandlerEntry{constants::bmcRedundancyServiceName,
                                 constants::bmcRedundancyObjectPath,
                                 constants::bmcRedundancyInterface, "Role",
                                 &SystemStatus::handleBmcRole},
        };

        const auto it = std::ranges::find_if(handlers, [&](const auto& entry) {
            return entry.service == service && entry.objectPath == objectPath &&
                   entry.interface == interface && entry.property == property;
        });

        if (it == handlers.end())
        {
            lg2::error("No handler for [{SVC}] [{PATH}] [{INTF}] [{PROP}]",
                       "SVC", service, "PATH", objectPath, "INTF", interface,
                       "PROP", property);
            return;
        }

        (this->*(it->handler))(value);
    }
    catch (const std::exception& ex)
    {
        lg2::error("Handler failed for {INTF} {PROP}, reason: {ERROR}", "INTF",
                   interface, "PROP", property, "ERROR", ex);
    }
}

bool SystemStatus::isServiceRunning(const std::string& service) noexcept
{
    try
    {
        if (service.empty())
        {
            lg2::error("Service name is empty, cannot check if it is running");
            return false;
        }

        auto method = connection->new_method_call(
            constants::dbusService, constants::dbusObjectPath,
            constants::dbusInterface, "NameHasOwner");
        method.append(service);

        auto reply = connection->call(method);

        bool hasOwner = false;
        reply.read(hasOwner);
        return hasOwner;
    }
    catch (const std::exception& ex)
    {
        lg2::error("Failed to check if service {SVC} is running, error: "
                   "{ERROR}",
                   "SVC", service, "ERROR", ex);
        return false;
    }
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
    const std::string& service, const std::string& objectPath,
    const std::string& interface, const nlohmann::json& properties,
    types::SystemStatusInfo& statusInfo)
{
    bool readAny = false;

    for (const auto& aProperty : properties)
    {
        const std::string propertyName = aProperty.template get<std::string>();

        // Extract system parameters from BaseBIOSTable
        if (propertyName == "BaseBIOSTable")
        {
            const auto readResult = utils::readSystemParametersFromBiosTable();
            if (!readResult)
            {
                lg2::error(
                    "Failed to read BaseBIOSTable system parameters: skipping");
                continue;
            }

            statusInfo.operatingMode = std::get<1>(*readResult);
            readAny = true;
            continue;
        }

        const auto result = utils::readDbusProperty(service, objectPath,
                                                    interface, propertyName);
        if (!result)
        {
            continue;
        }

        const auto* value = std::get_if<std::string>(&result.value());
        if (!value)
        {
            lg2::error("D-Bus property [{PROP}] is not a string: skipping",
                       "PROP", propertyName);
            continue;
        }

        if (propertyName == "BootProgress")
        {
            statusInfo.bootProgress = *value;
        }
        else if (propertyName == "CurrentBMCState")
        {
            statusInfo.currentBmcState = *value;
        }
        else if (propertyName == "CurrentPowerState")
        {
            statusInfo.currentPowerState = *value;
        }
        else if (propertyName == "Role")
        {
            statusInfo.bmcRole = *value;
        }
        else
        {
            lg2::error("Unknown property [{PROP}]: skipping", "PROP",
                       propertyName);
            continue;
        }

        readAny = true;
    }

    return readAny;
}
} // namespace panel
