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
     * @param[in] iface - Pointer to dbus interface class.
     */
    BusHandler(std::shared_ptr<sdbusplus::asio::dbus_interface>& iface) :
        iface(iface)
    {
        iface->register_method("Display", [this](const std::string& line1,
                                                 const std::string& line2) {
            this->display(line1, line2);
        });
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

    /* Pointer to interface */
    std::shared_ptr<sdbusplus::asio::dbus_interface> iface;
};

} // namespace panel
