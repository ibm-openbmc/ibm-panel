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
 * @brief An api to read System operating mode.
 * It will use combination of below mentioned three parameters to
 * decide the operating mode of the system.
 *
 * By default it will be "Normal " mode.
 * Value of three parameters in "Manual" mode would be,
 * QuiesceOnHwError - true.
 * PowerRestorePolicy -
 * "xyz.openbmc_project.Control.Power.RestorePolicy.Policy.AlwaysOff".
 * AutoReboot - false.
 *
 * Any other value combination will set the system to "Normal" mode.
 *
 * @param[out] sysOperatingMode - Operating mode.
 */
void readSystemOperatingMode(std::string& sysOperatingMode);

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

/**
 * @brief Make mapper call to get boot side paths.
 * @return List of all image object paths.
 */
std::vector<std::string> getBootSidePaths();

/**
 * @brief Get next marked boot side.
 * @param[out] nextBootSide -  Next selected boot side.
 */
void getNextBootSide(std::string& nextBootSide);

/**
 * @brief Api which sends lamp test command to the panel.
 * @param[in] transport - shared pointer object to transport class.
 */
void doLampTest(std::shared_ptr<Transport>& transport);

/**
 * @brief Api to restore the current display state on panel.
 * @param[in] transport - shared pointer object to transport class to send the
 * display lines.
 */
void restoreDisplayOnPanel(std::shared_ptr<Transport>& transport);

/**
 * @brief Find and retrieve the PDR.
 * This api returns the pdr for the given terminusId, entityId and
 * stateSetId.
 *
 * @param[in] terminusId - PLDM terminus id.
 * @param[in] entityId - Id representing an entity associated to the given
 * PLDM state set.
 * @param[in] stateSetId - Id representing PLDM state set.
 * @param[in] pdrMethod - PDR method name
 * (FindStateEffecterPDR/FindStateSensorPDR).
 *
 * @return PDR data.
 */
types::PdrList getPDR(const uint8_t& terminusId, const uint16_t& entityId,
                      const uint16_t& stateSetId, const std::string& pdrMethod);

/**
 * @brief Get sensor data like sensor id from the PDR.
 * @param[in] stateSensorPdr - sensor PDR.
 * @param[out] sensorId - sensor id fetched from sensor PDR.
 * PDR.
 */
void getSensorDataFromPdr(const types::PdrList& stateSensorPdr,
                          uint16_t& sensorId);

} // namespace utils
} // namespace panel