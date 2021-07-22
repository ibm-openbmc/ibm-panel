#pragma once

#include "transport.hpp"

#include <memory>
#include <sdbusplus/asio/object_server.hpp>
#include <string>
namespace panel
{
/** @class PanelPresence
 * @brief Panel presence method implementation.
 *
 * Class which has methods to listen to the panel's "Present" property change
 * event and set the transport key based on the panel presence.
 */
class PanelPresence
{
  public:
    PanelPresence(const PanelPresence&) = delete;
    PanelPresence& operator=(const PanelPresence&) = delete;
    PanelPresence(PanelPresence&&) = delete;
    ~PanelPresence() = default;

    /** A Constructor
     * Constructor which instantiates the PanelPresence class object.
     * @param[in] objPath - panel's dbus object path.
     * @param[in] conn - panel sdbusplus connection object.
     * @param[in] transportObj - transport object to set the transport key.
     */
    PanelPresence(std::string& objPath,
                  std::shared_ptr<sdbusplus::asio::connection> conn,
                  std::shared_ptr<panel::Transport> transportObj) :
        objectPath(objPath),
        conn(conn), transportObj(transportObj)
    {
    }

    /** @brief Listen to the Panel Present property.
     * Match signal is created to continuously listen to dbus
     * "Properties.Changed" signal of panel's Present property.
     */
    void listenPanelPresence();

  private:
    std::string objectPath;
    std::shared_ptr<sdbusplus::asio::connection> conn;
    std::shared_ptr<panel::Transport> transportObj;

    /** @brief Read panel's "Present" property and set the transport key.
     * @param[in] msg - pointer to the msg sent by the PropertiesChanged signal.
     */
    void readPresentProperty(sdbusplus::message::message& msg);
};
} // namespace panel