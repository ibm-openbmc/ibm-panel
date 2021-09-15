#pragma once

#include "transport.hpp"

#include <sdbusplus/asio/object_server.hpp>

namespace panel
{
/**
 * @brief A class to handle bus calls.
 * This class will handle calls made to the panel over D-Bus and register
 * method for Dbus interface.
 */
class BusHandler
{
  public:
    /* Deleted APIs */
    BusHandler(const BusHandler&) = delete;
    BusHandler(BusHandler&&) = delete;
    BusHandler& operator=(const BusHandler&) = delete;

    /**
     * @brief Constructor.
     * @param[in] transport - Transport class object.
     * @param[in] iface - Pointer to Panel dbus interface.
     */
    BusHandler(std::shared_ptr<Transport>& transport,
               std::shared_ptr<sdbusplus::asio::dbus_interface>& iface) :
        transport(transport),
        iface(iface)
    {
        iface->register_method("Display", [this](const std::string& line1,
                                                 const std::string& line2) {
            this->display(line1, line2);
        });

        iface->register_method(
            "TriggerPanelLampTest",
            [this](const bool status) { this->triggerPanelLampTest(status); });
    }

  private:
    /**
     * @brief Api to display on LCD panel.
     * This api can be called over Dbus to display any content
     * on LCD display as per panel LCD display specifications.
     *
     * @param[in] displayLine1: 1st line of display.
     * @param[in] displayLine2: 2nd line of display.
     *
     *
     */
    void display(const std::string& displayLine1,
                 const std::string& displayLine2);

    /**
     * @brief Dbus API to trigger panel lamp test.
     * @param[in] state - If state is true, the panel lamp test command will be
     * sent to the panel micro code. If false, then default function 01
     * execution happens.
     */
    void triggerPanelLampTest(const bool state);

    /* Pointer to transport class */
    std::shared_ptr<Transport> transport;

    /* Pointer to interface */
    std::shared_ptr<sdbusplus::asio::dbus_interface> iface;
};

} // namespace panel
