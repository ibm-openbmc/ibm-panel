#pragma once

#include "transport.hpp"
#include "types.hpp"

#include <memory>
#include <string>

namespace panel
{

/** @class StateManager
 *
 *  @brief State handler for the Op-Panel
 *
 *  Holds panel state information and tracks the current active functionality
 *  at which the panel is operating.
 */
class StateManager
{
  public:
    /** @brief Deleted special members */
    StateManager(const StateManager&) = delete;
    StateManager& operator=(const StateManager&) = delete;
    StateManager(StateManager&&) = delete;
    StateManager& operator=(StateManager&&) = delete;

    ~StateManager() = default;

    /**
     * @brief Parameterized constructor
     *
     * @param[in] transport - Transport object used to communicate with the
     * panel.
     @param[in] role      - Role FunctionMask based on the system IM value.
     *
     * @note @p role initialises systemState to `constants::roleUnknow`
     * (Unknown mode — when role is not yet determined) for high-end
     * redundant-BMC systems, or `constants::roleMask` for mid-range and low-end
     * systems where the BMC role is not applicable, so the function is always
     * enabled/disabled based solely on other state bits.
     *
     * @throws std::runtime_error
     */
    StateManager(std::shared_ptr<Transport> transport,
                 types::FunctionMask role) :
        transport(transport), systemState(role)
    {
        if (!transport)
        {
            throw std::runtime_error("Transport pointer is null");
        }

        initPanelState();
    }

  private:
    /**
     * @brief Initializes panel state members to their default values
     *
     * Populates @ref panelFunctions from the static functionality list,
     * sets @ref panelCurState to INITIAL_STATE, seeds @ref panelCurSubStates
     * with its initial and invalid sentinel values.
     */
    void initPanelState() noexcept;

    /** @brief Shared pointer to the Transport object */
    std::shared_ptr<Transport> transport;

    /**
     * @brief Stores information related to a particular functionality
     *
     * It will carry information like function number, its
     * subrange etc. It will also store active state of a functionality at a
     * given point of time during execution.
     */
    struct PanelFunctionality
    {
        types::FunctionNumber functionNumber =
            0; /**< Serial number of the function. */
        bool functionActiveState =
            false;                /**< true if enabled, false if disabled. */
        std::string debouceSrc{}; /**< Debounce source string. */
        types::FunctionNumber subFunctionUpperRange; /**< Upper bound of the
                                                        sub-function range. */
        types::FunctionMask functionEnableMask =
            0x00; /**< Bitmask of conditions required to enable this function.
                   */
        types::FunctionMask functionEnabledByPhyp =
            0x00; /**< Non-zero if this function requires enabling by PHYP. */
    };

    /** @brief List of all functions provided by the panel */
    std::vector<PanelFunctionality> panelFunctions;

    /** @brief Current state of the panel
     *
     *  Stores the index into @ref panelFunctions whose function number
     *  represents the current active functionality.
     */
    types::Byte panelCurState;

    /** @brief Current sub-state stack
     *
     *  Each index corresponds to a nesting level of the sub-state hierarchy.
     */
    types::FunctionalityList panelCurSubStates;

    /**
     * @brief Bitmask tracking the state of system modules that control
     *        enabling or disabling of panel functions.
     *
     * Each bit represents the state of a particular module or condition.
     * Bit layout (Big Endian):
     *  - Bit 0 : Reserved. PHYP enable bit set in the function structure.
     *  - Bit 1 : Operation mode — Normal(0) / Manual(1)
     *  - Bit 2 : Power state — Off(0) / On(1)
     *  - Bit 3 : PHYP runtime — No(0) / Yes(1)
     *  - Bit 4 : CE mode — No(0) / Yes(1)
     *  - Bit 5 : BMC standby — Not ready(0) / Ready(1)
     *  - Bit 6 : Unknow mode — BMC role is not yet determined.
     *  - Bit 7 : Passive mode — BMC role is Passive.
     *  - Bit 8 : Active mode — BMC role is Active.
     *  - Bits 9-15 : Reserved
     *
     * Bits 6-8 only applicable on redundant-BMC systems. On mid-range and
     * low-end systems BMC role is not applicable.
     */
    types::FunctionMask systemState = 0;
}; // class StateManager

} // namespace panel
