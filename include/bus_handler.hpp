#pragma once

#include "panel_state_manager.hpp"
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
     * @param[in] manager - Pointer to state manager.
     * @param[in] executor - Pointer to executor.
     */
    BusHandler(std::shared_ptr<Transport>& transport,
               std::shared_ptr<sdbusplus::asio::dbus_interface>& iface,
               std::shared_ptr<state::manager::PanelStateManager>& manager,
               std::shared_ptr<Executor>& executor) :
        transport(transport),
        iface(iface), stateManager(manager), executor(executor)
    {
        iface->register_method("Display", [this](const std::string& line1,
                                                 const std::string& line2) {
            this->display(line1, line2);
        });

        iface->register_method("TriggerPanelLampTest",
                               [this](const bool status) {
            this->triggerPanelLampTest(status);
        });

        iface->register_method("toggleFunctionState",
                               [this](const types::FunctionalityList& list) {
            this->toggleFunctionState(list);
        });

        iface->register_property(
            "ACFWindowActive", false,
            sdbusplus::asio::PropertyPermission::readWrite);

        iface->register_method("ProcessButton",
                               [this](int event) { this->btnRequest(event); });

        iface->register_property("OSIPLMode", false,
                                 [this](const bool newVal, bool& oldVal) {
            if (newVal != oldVal)
            {
                this->executor->setOSIPLMode(newVal);
                oldVal = newVal;
                return 1;
            }
            return 0;
        });

        iface->register_method("ExecuteFunction",
                               [this](const types::FunctionNumber funcNum) {
            return this->triggerPanelFunc(funcNum);
        });

        iface->register_method("getEnabledFunctions", [this]() {
            return this->getEnabledFunctionsList();
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

    /**
     * @brief Dbus API to trigger panel lamp test.
     * @param[in] state - If state is true, the panel lamp test command will be
     * sent to the panel micro code. If false, then default function 01
     * execution happens.
     */
    void triggerPanelLampTest(const bool state);

    /**
     * @brief An api to toggle panel function state.
     * Ths api is exposed on bus and is to be used to enable/disable panel
     * functions.
     * @param[in] list - A byte array, where each bit corresponds to a panel
     * function.
     */
    void toggleFunctionState(types::FunctionalityList list);

    /* Pointer to transport class */
    std::shared_ptr<Transport> transport;

    /* Pointer to interface */
    std::shared_ptr<sdbusplus::asio::dbus_interface> iface;

    /* Pointer to state manager class */
    std::shared_ptr<state::manager::PanelStateManager> stateManager;

    /* Pointer to Executor class */
    std::shared_ptr<Executor> executor;

    /**
     * @brief Api to process button request.
     *
     * @param[in] event: Button event
     */
    void btnRequest(int event);

    /**
     * @brief API to execute panel function number.
     *
     * Method to trigger panel function from external source.
     *
     * @param[in] funcNum - Function number to execute
     * @return a tuple with status(true/false) and display lines.
     */
    types::ReturnStatus triggerPanelFunc(const types::FunctionNumber funcNum);

    /**
     * @brief API to get list of enabled functions.
     *
     * @return List of enabled functions.
     */
    types::Binary getEnabledFunctionsList();
};

} // namespace panel
