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
                  std::shared_ptr<Transport> transport,
                  std::shared_ptr<state::manager::PanelStateManager> state) :
        objectPath(objPath),
        conn(conn), transport(transport), stateManager(state)
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
    std::shared_ptr<state::manager::PanelStateManager> stateManager;

    /** @brief Read panel's "Present" property and set the transport key.
     * @param[in] msg - pointer to the msg sent by the PropertiesChanged signal.
     */
    void readPresentProperty(sdbusplus::message::message& msg);

    /**
     * @brief Base panel "Present" property callback.
     * It resets LEDs state on base panel and sets the transport key.
     *
     * @param[in] msg - pointer to the msg sent by the PropertiesChanged signal.
     */
    void readBasePresentProperty(sdbusplus::message::message& msg);
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
     * @param[in] transport - pointer to transport class.
     */
    PELListener(std::shared_ptr<sdbusplus::asio::connection> con,
                std::shared_ptr<state::manager::PanelStateManager> manager,
                std::shared_ptr<Executor> execute,
                std::shared_ptr<Transport>& transport) :
        conn(con),
        stateManager(manager), executor(execute), transport(transport)
    {
    }

    /**
     * @brief Api to listen for PEL addition/deletion events.
     */
    void listenPelEvents();

  private:
    /* Callback to listen for PEL event log */
    void PELEventCallBack(sdbusplus::message::message& msg);

    /* Callback to listen for PEL Delete event log */
    void PELDeleteEventCallBack(sdbusplus::message::message& msg);

    /**
     * @brief An Api to set panel function state based on PEL data.
     * @param[in]pelObjPath - Object path of the PEL logged.
     */
    void setPelRelatedFunctionState(
        const sdbusplus::message::object_path& pelObjPath);

    /**
     * @brief An Api to get list of PELs logged in the system.
     */
    void getListOfExistingPels();

    /**
     * @brief An Api to filter PELs of desired severity and eventId.
     * @param[in] listOfPels - List of existing PELs in the system.
     */
    void filterPel(const types::GetManagedObjects& listOfPels);

    /* Dbus connection */
    std::shared_ptr<sdbusplus::asio::connection> conn;

    /* state manager */
    std::shared_ptr<state::manager::PanelStateManager> stateManager;

    /* Executor */
    std::shared_ptr<Executor> executor;

    /* pointer to Transport class*/
    std::shared_ptr<Transport> transport;

    /* Check if respective functions are enabled */
    bool functionStateEnabled = false;

    /* Store the last logged PEL with required severity */
    std::string lastPelObjPath;

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
     * @param[in] execute - pointer to Executor.
     */
    BootProgressCode(std::shared_ptr<Transport> transport,
                     std::shared_ptr<sdbusplus::asio::connection> con,
                     std::shared_ptr<Executor> execute) :
        transport(transport),
        conn(con), executor(execute)
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

    /* Executor */
    std::shared_ptr<Executor> executor;

}; // class BootProgressCode

/**
 * @brief Class to track system status.
 * This class implements handler to listen for system parameters to
 * know the current state and operating mode of the system as a whole.
 */
class SystemStatus
{
  public:
    SystemStatus(const SystemStatus&) = delete;
    SystemStatus& operator=(const SystemStatus&) = delete;
    SystemStatus(SystemStatus&&) = delete;
    SystemStatus& operator=(const SystemStatus&&) = delete;
    ~SystemStatus() = default;

    /**
     * @brief Constructor.
     * @param[in] con - Bus connection.
     * @param[in] manager - Pointer to state manager.
     */
    SystemStatus(std::shared_ptr<sdbusplus::asio::connection>& con,
                 std::shared_ptr<state::manager::PanelStateManager>& manager);

  private:
    /**
     * @brief Listen for BMC state.
     * An api to register call back for BMC state property changed.
     */
    void listenBmcState();

    /**
     * @brief Listen for power state.
     * An api to register call back for chassis power state change.
     */
    void listenPowerState();

    /**
     * @brief Listen for boot progress state.
     * An api to register call back for boot progress state change.
     */
    void listenBootProgressState();

    /**
     * @brief Listen for system operating mode parameters.
     * An api to register call back for system operating mode change.
     */
    void listenSystemOperatingModeParameters();

    /**
     * @brief Api to handle BMC state change callback.
     * @param[in] msg - Callback message.
     */
    void bmcStateCallback(sdbusplus::message::message& msg);

    /**
     * @brief Api to handle power state change callback.
     * @param[in] msg - Callback message.
     */
    void powerStateCallback(sdbusplus::message::message& msg);

    /**
     * @brief Api to handle boot progress state change callback.
     * @param[in] msg - Callback message.
     */
    void bootProgressStateCallback(sdbusplus::message::message& msg);

    /**
     * @brief Api to handle power policy state change callback.
     * @param[in] msg - Callback message.
     */
    void powerPolicyStateCallback(sdbusplus::message::message& msg);

    /**
     * @brief Api to handle reboot policy state change callback.
     * @param[in] msg - Callback message.
     */
    void rebootPolicyStateCallback(sdbusplus::message::message& msg);

    /**
     * @brief Api to initialize system operating parameters.
     */
    void initSystemOperatingParameters();

    /**
     * @brief Set system operating mode.
     * An api to set system operating mode based on the value of three
     * parameters required to detect operating mode.
     * a) Logging Setting.
     * b) Power policy.
     * c) Reboot policy.
     */
    void setSystemCurrentOperatingMode();

    /* D-Bus connection. */
    std::shared_ptr<sdbusplus::asio::connection> conn;

    /* state manager */
    std::shared_ptr<state::manager::PanelStateManager> stateManager;

    /* Member to store power policy */
    std::string powerPolicy;

    /* Member to store reboot policy */
    bool rebootPolicy;
};
} // namespace panel