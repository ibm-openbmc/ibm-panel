#pragma once

#include "executor.hpp"
#include "transport.hpp"
#include "types.hpp"

#include <memory>
#include <tuple>

namespace panel
{
namespace state
{
namespace manager
{

/** @class PanelStateManager
 *  @brief Class to implement state handler for Op-Panel.
 * It will hold Op-Panel state information.
 * State of an Op-Panel indicates the current functionality at which the
 * panel is.
 */
class PanelStateManager
{
  public:
    /* Deleted Api's*/
    PanelStateManager(const PanelStateManager&) = delete;
    PanelStateManager& operator=(const PanelStateManager&) = delete;
    PanelStateManager(PanelStateManager&&) = delete;

    ~PanelStateManager() = default;

    /**
     * @brief Api to reset state of Op-Panel to default.
     * */
    inline void resetPanelState()
    {
        initPanelState();
    }

    /**
     * @brief Api to reset state manager.
     * This needs to be called when we want to reset the state manager and LCD
     * panel to function 01.
     */
    void resetStateManager();

    /**
     * @brief Api to get state and sub state info of panel.
     * */
    std::tuple<types::FunctionNumber, types::FunctionNumber>
        getPanelCurrentStateInfo() const;

    /**
     * @brief Api to enable a set of functionality.
     * @param[in] listOfFunctionalities - A vector of functionalities to be
     * enabled. It will basically contain just the functionality number
     * corresponding to a functionality.
     * */
    void enableFunctonality(
        const types::FunctionalityList& listOfFunctionalities);

    /**
     * @brief API to toggle function(s) state from Phyp.
     * This api will be called when phyp enables/diables any function(s) from
     * its end.
     * @param[in] list - A byte array, where each bit corresponds to a panel
     * function.
     */
    void toggleFuncStateFromPhyp(const types::FunctionalityList& list);

    /**
     * @brief Api to disable function(s).
     * @param[in] listOfFunctionalities - A list of function(s) to be disabled.
     * */
    void disableFunctonality(
        const types::FunctionalityList& listOfFunctionalities);

    /**
     * @brief Api to process button event.
     * This api will be called in case of any button event, will process and
     * set the state of panel accordingly.
     * @param[in] button - button event.
     */
    void processPanelButtonEvent(const types::ButtonEvent& button);

    /**
     * @brief Api to set state of Panel state handler altogether.
     * @param[in] isEnabled - boolean value to enable/disable panel state
     * handler.
     */
    inline void setPanelState(bool isEnabled)
    {
        isPanelStateActive = isEnabled;
    }

    /**
     * @brief Constructor.
     * @param[in] transport - transport object to call transport functions
     * @param[in] execute - pointer to executor class.
     */
    PanelStateManager(std::shared_ptr<Transport> transport,
                      std::shared_ptr<Executor> execute) :
        transport(transport),
        funcExecutor(execute)
    {
        initPanelState();
    }

    /**
     * @brief Api to toggle bmc state bit in member variable "systemState".
     * @param[in] bmcState - Bmc state.
     */
    void updateBMCState(const std::string& bmcState);

    /**
     * @brief Api to toggle power state bit in member variable "systemState".
     * @param[in] powerState - Power state.
     */
    void updatePowerState(const std::string& powerState);

    /**
     * @brief Api to toggle boot progress state bit.
     * @param[in] bootState - Boot progress state.
     */
    void updateBootProgressState(const std::string& bootState);

    /**
     * @brief Api to set system operating mode.
     * @param[in] operatingMode - Current mode of system.
     */
    void setSystemOperatingMode(const std::string& operatingMode);

    /**
     * @brief An api to enable/disable CE mode.
     */
    void setCEState();

    /**
     * @brief API to trigger functions on request from external source
     * @param[in] funcNum - Function number
     * @return status of the function
     */
    types::ReturnStatus
        triggerFunctionDirectly(const types::FunctionNumber funcNum);

    /**
     * @brief API to get list of enabled functions
     *
     * @return List of enabled functions
     */
    types::Binary getEnabledFunctionsList();

  private:
    /**
     * @brief An Api to set the initial state of PanelState class.
     * It will set the initial state, substate and other class members to
     * their initial values respectively.
     */
    void initPanelState();

    /**
     * @brief An Api to increment Op-Panel state.
     */
    void incrementState();

    /**
     * @brief An Api to decrement Op-Panel state.
     */
    void decrementState();

    /**
     * @brief An Api to execute current Op-Panel state.
     */
    void executeState();

    /**
     * @brief An api to set IPL type, system operating mode and firmware
     * mode.
     * @param[in] button - button event.
     */
    void setIPLParameters(const types::ButtonEvent& button);

    /** @brief API to create the display string. */
    void createDisplayString() const;

    /**
     * @brief An api to display panel states on standard out.
     * This is mainly being used for debug.
     */
    void printPanelStates();

    /**
     * @brief Api which displays debounce src.
     * This api is called when the current function's execute needs to display
     * debounce src. The debounce src is taken from the functionalityList.
     */
    void displayDebounce() const;

    /** @brief Api to display function 2 on panel */
    void displayFunc02();

    /**
     * @brief An api to initialise function 02 with its initial values.
     * It will read the respective values from Dbus and set initial value of
     * functionality 02 accordingly.
     */
    void initFunction02();

    /**
     * @brief An api to toggle state of panel functons.
     * This api will be called everytime any bit changes in system state to
     * check if any panel function can be enabled/disabled based on that.
     */
    void updateFunctionStatus();

    /**
     * @brief API which tells if the function is enabled or not
     * @param[in] funcNum - Function number
     * @return (true/false) if function is enabled/disabled respectively.
     */
    bool isFunctionEnabled(const types::FunctionNumber funcNum);

    /**
     * @brief Check if function is supported to execute from d-bus
     * @param[in] funcNum - Function number
     * @return true if function is supported, false otherwise.
     */
    bool isFunctionSupported(const types::FunctionNumber funcNum);
    /**
     * @brief A structure to store information related to a particular
     * functionality. It will carry information like function number, its
     * subrange etc. It will also store active state of a functionality at a
     * given point of time during execution.
     */
    struct PanelFunctionality
    {
        // serial number of the function.
        types::FunctionNumber functionNumber = 0;

        // true is enabled false is disabled.
        bool functionActiveState = false;

        // debounce string.
        std::string debouceSrc{};

        // Upper range in sub function list.
        types::FunctionNumber subFunctionUpperRange;

        // conditions to enable a particular function.
        types::Byte functionEnableMask = 0x00;

        // If function require enabling by PHYP.
        types::Byte functionEnabledByPhyp = 0x00;
    };

    // A list of functions provided by the panel.
    std::vector<PanelFunctionality> panelFunctions;

    // To store current state of Op-Panel. This will store the index of
    // vector panelFunctions. Fetch function number at that index to get the
    // current active functionality.
    uint8_t panelCurState;

    // To store substate's state. In case of nested substate, every index
    // refer to the level of substate we are at.
    types::FunctionalityList panelCurSubStates;

    // A variable to keep track if sub range is active. Required to decide
    // if we need to loop in subrange or not.
    bool isSubrangeActive = false;

    // Hold information if state machine is in enabled state or not.
    bool isPanelStateActive = false;

    const std::vector<std::vector<std::string>> functionality02 = {
        {{"A", "B", "C", "D"}, {"M", "N"}, {"P", "T"}}};

    std::shared_ptr<Transport> transport;

    /** The current active sub level of function 2 */
    uint8_t levelToOperate = 0;

    /*shared pointer to executor object*/
    std::shared_ptr<Executor> funcExecutor;

    /* Boot side selected for next boot.*/
    std::string nextBootSideSelected = "P";

    /* A variable to store state of specific modules required to enable/disable
     * panel functions.
     * Each bit represent state of a particular module.
     * Bitwise AND this variable with bit mask of that function from table to
     * enable/disable the function.
     * Representation as in Big Endian.
     * 0th bit - Reserved. PHYP enable bit set in the function structure.
     * 1st bit - Operation mode Normal/Manual 0/1
     * 2nd bit - Power on state On/Off 1/0
     * 3rd bit - Is Runtime No/Yes 0/1
     * 4th bit - CE No/Yes 0/1
     * 5th bit - At standby No/Yes 0/1
     * 6th bit - Reserved.
     * 7th bit - Reserved.
     */
    types::Byte systemState = 0;
}; // class PanelStateManager

} // namespace manager
} // namespace state
} // namespace panel
