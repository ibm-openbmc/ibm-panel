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
std::string binaryToHexString(const panel::types::Binary& val);

/** @brief Display on panel using transport class api.
 *
 * Method which sends the actual data to the panel's micro code using Transport
 * class write, to display the data on lcd panel.
 * TODO: Enable scroll if the lines exceeds 16 characters.
 *
 * @param[in] line1 - line 1 data that needs to be displayed.
 * @param[in] line2 - line 2 data that needs to be displayed.
 * @param[in] transportObj - Transport class object to access panelI2CWrite
 * method.
 */
void sendCurrDisplayToPanel(const std::string& line1, const std::string& line2,
                            std::shared_ptr<panel::Transport> transportObj);
} // namespace utils
} // namespace panel
