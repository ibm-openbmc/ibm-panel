#include "pldm_dbus_signals.hpp"

namespace panel
{
void PLDMSensorEvents::listenStateSensorEvent()
{
    // Matches on the pldm StateSensorEvent signal
    pldmEventSignal = std::make_unique<sdbusplus::bus::match::match>(
        *bus,
        sdbusplus::bus::match::rules::type::signal() +
            sdbusplus::bus::match::rules::member("StateSensorEvent") +
            sdbusplus::bus::match::rules::path("/xyz/openbmc_project/pldm") +
            sdbusplus::bus::match::rules::interface(
                "xyz.openbmc_project.PLDM.Event"),
        std::bind_front(&PLDMSensorEvents::stateSensorCallback, this));
}

void PLDMSensorEvents::stateSensorCallback(sdbusplus::message::message& msg)
{
    uint8_t msgTID;
    uint16_t msgSensorID;
    uint8_t msgSensorOffset;
    uint8_t msgEventState;
    uint8_t msgPreviousEventState;

    // Read the msg and populate each variable
    msg.read(msgTID, msgSensorID, msgSensorOffset, msgEventState,
             msgPreviousEventState);

    if (msgTID == constants::phypTerminusID && msgSensorID == osIPLModeSensorId)
    {
        osIPLModeState = msgEventState;
    }
}

} // namespace panel