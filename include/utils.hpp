#pragma once
#include <phosphor-logging/lg2.hpp>
#include <sdbusplus/asio/object_server.hpp>
#include <string>

namespace panel
{
namespace utils
{

/** @brief Read inventory manager properties from dbus.
 * @param[in] service - Dbus service name
 * @param[in] obj - Dbus object to query for the property.
 * @param[in] inf - Interface in which the property is present.
 * @param[in] prop - Property to be queried.
 * @return The property value in string.
 */
template <typename T>
inline T readBusProperty(const std::string& service, const std::string& object,
                         const std::string& inf,
                         const std::string& prop) noexcept
{
    T retVal{};
    try
    {
        auto bus = sdbusplus::bus::new_default();
        auto properties =
            bus.new_method_call(service.c_str(), object.c_str(),
                                "org.freedesktop.DBus.Properties", "Get");
        properties.append(inf);
        properties.append(prop);
        auto result = bus.call(properties);
        result.read(retVal);
    }
    catch (const sdbusplus::exception::SdBusError& e)
    {
        lg2::error("Read Dbus property call failed: {ERROR}", "ERROR",
                   e.what());
    }
    return retVal;
}

/**
 * @brief Determine the redundancy role of the BMC from D-Bus.
 *
 * This API queries the BMC redundancy service to fetch the current Role
 * property.
 *
 * Note: If there is any D-Bus error or if the service is not running, it
 * returns the default Unknown role.
 *
 * @return The BMC redundancy role.
 */
inline std::string getBmcRedundancyRole() noexcept
{
    try
    {
        auto retValue = readBusProperty<std::variant<std::string>>(
            constants::redundancyService, constants::redundancyObjectPath,
            constants::redundancyInterface, constants::roleProperty);

        const auto bmcRole = std::get_if<std::string>(&retValue);
        if (!bmcRole)
        {
            lg2::error("Invalid data received for BMC role from DBus, "
                       "returning role as Unknown.");
        }
        else if (bmcRole->empty())
        {
            lg2::error("Empty data received for BMC role from DBus, "
                       "returning role as Unknown.");
        }
        else
        {
            return *bmcRole;
        }
    }
    catch (const std::exception& ex)
    {
        lg2::error("Failed to read BMC redundancy role, error: {ERROR}. "
                   "Returning role as Unknown.",
                   "ERROR", ex.what());
    }
    return std::string(constants::roleUnknown);
}

/**
 * @brief Map a BMC redundancy role string to a panel operating mode name.
 *
 * Translates the raw D-Bus BMC redundancy role value into the human-readable
 * panel mode.
 *
 * The mapping is as follows:
 * | Role (D-Bus value)                                        | Mode     |
 * |-----------------------------------------------------------|----------|
 * | xyz.openbmc_project.State.BMC.Redundancy.Role.Passive     | Passive  |
 * | xyz.openbmc_project.State.BMC.Redundancy.Role.Active      | Active   |
 * | xyz.openbmc_project.State.BMC.Redundancy.Role.Unknown     | Safe     |
 *
 * @param[in] role - The BMC redundancy role string.
 *
 * @return A non-empty mode string ("Active", "Passive", or "Safe") when
 *         given role matches a known value; an empty string when no mapping
 *         exists for the supplied role.
 */
inline std::string getModeBasedOnBmcRole(const std::string& role)
{
    if (role.compare(constants::rolePassive) == 0)
    {
        return "Passive";
    }

    if (role.compare(constants::roleActive) == 0)
    {
        return "Active";
    }

    if (role.compare(constants::roleUnknown) == 0)
    {
        return "Safe";
    }
    return {};
}
} // namespace utils
} // namespace panel
