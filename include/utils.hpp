#pragma once
#include <const.hpp>
#include <sdbusplus/asio/object_server.hpp>
#include <sstream>
#include <string>
#include <transport.hpp>
#include <types.hpp>
#include <vector>

namespace panel
{
namespace utils
{
static types::PanelDataMap lcdDataMap = {
    {constants::rain2s2uIM,
     {constants::rainLcdDevPath, constants::devAddr,
      constants::rainLcdDbusObj}},
    {constants::rain2s4uIM,
     {constants::rainLcdDevPath, constants::devAddr,
      constants::rainLcdDbusObj}},
    {constants::rain1s4uIM,
     {constants::rainLcdDevPath, constants::devAddr,
      constants::rainLcdDbusObj}},
    {constants::everestIM,
     {constants::everLcdDevPath, constants::devAddr,
      constants::everLcdDbusObj}},
    {constants::balconesIM,
     {constants::balconesLcdDevPath, constants::devAddr,
      constants::balconesLcdDbusObj}},
    {constants::blueridge2s2uIM,
     {constants::blueridgeLcdDevPath, constants::devAddr,
      constants::blueridgeLcdDbusObj}},
    {constants::blueridge2s4uIM,
     {constants::blueridgeLcdDevPath, constants::devAddr,
      constants::blueridgeLcdDbusObj}},
    {constants::blueridge1s4uIM,
     {constants::blueridgeLcdDevPath, constants::devAddr,
      constants::blueridgeLcdDbusObj}},
    {constants::fujiIM,
     {constants::fujiLcdDevPath, constants::devAddr,
      constants::fujiLcdDbusObj}}};

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

/**
 * @brief Api to create PEL.
 *
 * @param[in] errIntf - Error Interface
 * @param[in] sev -  panel::constants::Severity of Error
 * @param[in] additionalData - Information of PEL
 */
void createPEL(const std::string& errIntf, const std::string& sev,
               const std::map<std::string, std::string>& additionalData);

/**
 * @brief Api to get service.
 *
 * @param[in] bus - bus input
 * @param[in] path -  Dbus object path
 * @param[in] interface - Interface
 */
std::string getService(sdbusplus::bus_t& bus, const std::string& path,
                       const std::string& interface);

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

/**
 * @brief Get subtree path for the given interface under given object path.
 * @param[in] objectPath - Object path to find the subtree for the given
 * interface.
 * @param[in] intf - Interface list.
 * @param[in] depth - Maximum subtree depth to fetch the results.
 *
 * @return List of all fru paths categorized under the given interface.
 */
std::vector<std::string> getSubTreePaths(const std::string& objectPath,
                                         const std::vector<std::string>& intf,
                                         const int32_t depth);

/**
 * @brief Get the systems IM value.
 *
 * @return the IM value of the system.
 */
std::string getSystemIM();

/**
 * @brief Get the presence of the LCD panel.
 * This api returns the value of the present property
 * for the LCD panel on the system interface.
 *
 * @param[in] imValue - IM value of the system.
 *
 * @return The value of the presence property.
 */
bool getLcdPanelPresentProperty(const std::string& imValue);

/**
 * @brief An API to get list of PELs and SRC logged in the system.
 *
 * @return The sorted list of object path and SRCs of last 25 PELs.
 */
types::PelPathAndSRCList geListOfPELsAndSRCs();

/**
 * @brief API to sort list of Pels.
 * This is required to pick last "n" number of PELs logged in the system.
 *
 * @param [in] listOfPels - list of PELs.
 */
void sortPels(types::GetManagedObjects& listOfPels);

/**
 * @brief API to return list of last 25 PELs.
 * Out of all the PELs retreived from the system, Panel needs to keep track of
 * PELs only with specific severity. Hence this API will filter those PELs from
 * the list of all the PELs.
 *
 * @param listOfPels - List of PELs retrieved from the system.
 * @param finalListOFPEls - List of PEL and SRC pair.
 */
void filterPel(const types::GetManagedObjects& listOfPels,
               types::PelPathAndSRCList& finalListOFPELs);

} // namespace utils
} // namespace panel
