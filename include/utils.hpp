#pragma once
#include <sdbusplus/asio/object_server.hpp>
#include <sstream>
#include <string>
#include <transport.hpp>
#include <types.hpp>

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
T readBusProperty(const std::string& service, const std::string& object,
                  const std::string& inf, const std::string& prop)
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
        std::cerr << e.what();
    }
    return retVal;
}

/** @brief Convert byte vector to hex string.
 * @param[in] val - byte vector that needs conversion
 * @return hex string
 */
std::string binaryToHexString(const types::Binary& val);

/** @brief Display on panel using transport class api.
 *
 * Method which sends the actual data to the panel's micro code using Transport
 * class write, to display the data on lcd panel.
 * TODO: Enable scroll if the lines exceeds 16 characters.
 *
 * @param[in] line1 - line 1 data that needs to be displayed.
 * @param[in] line2 - line 2 data that needs to be displayed.
 * @param[in] transport - Transport class object to access panelI2CWrite
 * method.
 */
void sendCurrDisplayToPanel(const std::string& line1, const std::string& line2,
                            std::shared_ptr<Transport> transport);

/**
 * @brief An api to read initial values of OS IPL types, System operating
 * mode, firmware IPL type, Hypervisor type and HMC indicator.
 * @return - Values of required system parameters.
 */
types::SystemParameterValues readSystemParameters();

/** @brief Make d-bus call to "GetManagedObjects" method
 * @param[in] service - service on which the d-bus call needs to happen.
 * @param[in] object - object path.
 * @return returns output of "GetManagedObjects" call.
 */
types::GetManagedObjects getManagedObjects(const std::string& service,
                                           const std::string& object);

/**
 * @brief An api to write Bus property.
 * @param[in] serviceName - Name of the service.
 * @param[in] objectPath - Object path
 * @param[in] infName - Interface name.
 * @param[in] propertyName - Name of the property whose value is being fetched.
 * @param[in] paramValue - The property value.
 */
template <typename T>
void writeBusProperty(const std::string& serviceName,
                      const std::string& objectPath, const std::string& infName,
                      const std::string& propertyName,
                      const std::variant<T>& paramValue)
{
    try
    {
        auto bus = sdbusplus::bus::new_default();
        auto method =
            bus.new_method_call(serviceName.c_str(), objectPath.c_str(),
                                "org.freedesktop.DBus.Properties", "Set");
        method.append(infName);
        method.append(propertyName);
        method.append(paramValue);

        bus.call(method);
    }
    catch (const sdbusplus::exception::SdBusError& e)
    {
        std::cerr << e.what();
        throw;
    }
}

} // namespace utils
} // namespace panel