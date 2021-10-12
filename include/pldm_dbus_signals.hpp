#pragma once

#include "const.hpp"
#include "utils.hpp"

#include <sdbusplus/asio/object_server.hpp>

namespace panel
{
/**
 * @brief Class to handle PLDM sensor events.
 * When the PLDM daemon receives a sensorEvent of type stateSensorState, it
 * emits the StateSensorEvent signal. This signal would be used by PLDM
 * requester apps on the BMC, which will rely on this signal to determine state
 * changes on a connected PLDM entity.
 */
class PLDMSensorEvents
{
  public:
    PLDMSensorEvents(const PLDMSensorEvents&) = delete;
    PLDMSensorEvents& operator=(const PLDMSensorEvents&) = delete;
    PLDMSensorEvents(PLDMSensorEvents&&) = delete;
    PLDMSensorEvents& operator=(const PLDMSensorEvents&&) = delete;
    ~PLDMSensorEvents() = default;

    /** @brief Default constructor for test case only. */
    PLDMSensorEvents()
    {
    }

    /** @brief Parameterised constructor.
     * @param[in] conn - Panel sdbusplus connection object.
     */
    PLDMSensorEvents(std::shared_ptr<sdbusplus::asio::connection>& conn) :
        bus(conn)
    {
        types::PdrList pdr = utils::getPDR(
            panel::constants::phypTerminusID, panel::constants::vmmEntityId,
            panel::constants::osIplModeStateSetId, "FindStateSensorPDR");
        utils::getSensorDataFromPdr(pdr, osIPLModeSensorId);
        listenStateSensorEvent();
    }

    /** @brief Is OS IPL mode setting enabled by PHYP or not.
     * This api returns if the OS IPL mode setting is enabled by PHYP or not.
     * @return true if enabled; false otherwise.
     */
    inline bool isOSIPLModeSettingEnabled()
    {
        return (osIPLModeState == 1);
    }

    /** @brief API to register PLDM state sensor event. */
    void listenStateSensorEvent();

  private:
    /** @brief Call back to the registered PLDM state sensor event.
     * @param[in] msg - Message received from the PLDM state sensor event.
     */
    void stateSensorCallback(sdbusplus::message::message& msg);

    /** Panel sdbusplus connection object. */
    std::shared_ptr<sdbusplus::asio::connection> bus;

    /** Match signal object for PLDM state sensor event. */
    std::unique_ptr<sdbusplus::bus::match::match> pldmEventSignal;

    /** OS IPL mode sensor ID defaults to 0. */
    uint16_t osIPLModeSensorId = 0;

    /** OS IPL Mode state by default disabled by PHYP with a value 2. */
    uint8_t osIPLModeState = 2;

}; // PLDMSensorEvents

} // namespace panel