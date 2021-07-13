#pragma once

#include "executor.hpp"
#include "panel_state_manager.hpp"
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
     * @param[in] transport - transport object to set the transport key.
     */
    PanelPresence(std::string& objPath,
                  std::shared_ptr<sdbusplus::asio::connection> conn,
                  std::shared_ptr<Transport> transport) :
        objectPath(objPath),
        conn(conn), transport(transport)
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
    std::shared_ptr<Transport> transport;

    /** @brief Read panel's "Present" property and set the transport key.
     * @param[in] msg - pointer to the msg sent by the PropertiesChanged signal.
     */
    void readPresentProperty(sdbusplus::message::message& msg);
};

/** @class PELListener
 * @brief Listen to PEL logged event.
 *
 * It will implement callback to listen for interface added against any PEL
 * event logged in the bmc.
 *
 */
class PELListener
{
  public:
    PELListener(const PELListener&) = delete;
    PELListener& operator=(const PELListener&) = delete;
    PELListener(PELListener&&) = delete;
    ~PELListener() = default;

    /**
     * @brief Constructor
     * @param[in] con - Bus connection.
     * @param[in] manager - Pointer to State manager.
     * @param[in] execute - pointer to Executor.
     */
    PELListener(std::shared_ptr<sdbusplus::asio::connection> con,
                std::shared_ptr<state::manager::PanelStateManager> manager,
                std::shared_ptr<Executor> execute) :
        conn(con),
        stateManager(manager), executor(execute)
    {
    }

    /**
     * @brief Api to listen for PEL events.
     */
    void listenPelEvents();

  private:
    /* Callback to listen for PEL event log */
    void PELEventCallBack(sdbusplus::message::message& msg);

    /* Dbus connection */
    std::shared_ptr<sdbusplus::asio::connection> conn;

    /* state manager */
    std::shared_ptr<state::manager::PanelStateManager> stateManager;

    /* Executor */
    std::shared_ptr<Executor> executor;

    /* Check if respective functions are enabled */
    bool functionStateEnabled = false;

}; // class PEL Listener

/**
 * @brief Progress code event handler.
 * A class to register handler for progress code property channge.
 */
class BootProgressCode
{
  public:
    BootProgressCode(const BootProgressCode&) = delete;
    BootProgressCode& operator=(const BootProgressCode&) = delete;
    BootProgressCode(BootProgressCode&&) = delete;
    ~BootProgressCode() = default;

    /**
     * @brief Constructor.
     * @param[in] transport - pointer to transport class.
     * @param[in] con - Bus connection.
     */
    BootProgressCode(std::shared_ptr<Transport> transport,
                     std::shared_ptr<sdbusplus::asio::connection> con) :
        transport(transport),
        conn(con)
    {
    }

    /**
     * @brief Api to register call back for progress code.
     */
    void listenProgressCode();

  private:
    /**
     * @brief Callback handler.
     * An Api to handle callback in case of progress code property change.
     * @param[in] msg - Callback message.
     */
    void progressCodeCallBack(sdbusplus::message::message& msg);

    /*Transport Class object */
    std::shared_ptr<Transport> transport;

    /* D-Bus connection. */
    std::shared_ptr<sdbusplus::asio::connection> conn;

}; // class BootProgressCode
} // namespace panel