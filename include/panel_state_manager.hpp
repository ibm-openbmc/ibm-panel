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

#ifdef StateManagerTest
    PanelStateManager() = default;
#endif

    ~PanelStateManager() = default;

    /**
     * @brief Api to reset state of Op-Panel to default.
     * */
    inline void resetPanelState()
    {
        initPanelState();
    }

    /**
     * @brief Api to get state and sub state info of panel.
     * */
    std::tuple<panel::types::FunctionNumber, panel::types::FunctionNumber>
        getPanelCurrentStateInfo() const;

    /**
     * @brief Api to enable a set of functionality.
     * @param[in] listOfFunctionalities - A vector of functionalities to be
     * enabled. It will basically contain just the functionality number
     * corresponding to a functionality.
     * */
    void enableFunctonality(
        const panel::types::FunctionalityList& listOfFunctionalities);

    /**
     * @brief Api to process button event.
     * This api will be called in case of any button event, will process and
     * set the state of panel accordingly.
     * @param[in] button - button event.
     */
    void processPanelButtonEvent(const panel::types::ButtonEvent& button);

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
     * @param[in] transport - transport object to call transport functions.
     */
    PanelStateManager(std::shared_ptr<panel::Transport> transport) :
        transport(transport)
    {
        initPanelState();
    }

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
    void setIPLParameters(const panel::types::ButtonEvent& button);

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
    void displayFunc02() const;

    /**
     * @brief A structure to store information related to a particular
     * functionality. It will carry information like function number, its
     * subrange etc. It will also store active state of a functionality at a
     * given point of time during execution.
     */
    struct PanelFunctionality
    {
        // serial number of the function.
        panel::types::FunctionNumber functionNumber = 0;

        // true is enabled false is disabled.
        bool functionActiveState = false;

        // debounce string.
        std::string debouceSrc{};

        // Upper range in sub function list.
        panel::types::FunctionNumber subFunctionUpperRange;
    };

    // A list of functions provided by the panel.
    std::vector<PanelFunctionality> panelFunctions;

    // To store current state of Op-Panel. This will store the index of
    // vector panelFunctions. Fetch function number at that index to get the
    // current active functionality.
    uint8_t panelCurState;

    // To store substate's state. In case of nested substate, every index
    // refer to the level of substate we are at.
    panel::types::FunctionalityList panelCurSubStates;

    // A variable to keep track if sub range is active. Required to decide
    // if we need to loop in subrange or not.
    bool isSubrangeActive = false;

    // Hold information if state machine is in enabled state or not.
    bool isPanelStateActive = false;

    const std::vector<std::vector<std::string>> functionality02 = {
        {{"A", "B", "C", "D"}, {"M", "N"}, {"P", "T"}}};

    std::shared_ptr<panel::Transport> transport;

    /** The current active sub level of function 2 */
    uint8_t levelToOperate = 0;

    /*shared pointer to executor object*/
    std::shared_ptr<panel::Executor> funcExecutor;
}; // class PanelStateManager

} // namespace manager
} // namespace state
} // namespace panel
