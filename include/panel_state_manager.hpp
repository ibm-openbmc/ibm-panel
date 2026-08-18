#pragma once

#include "transport.hpp"
#include "types.hpp"

#include <memory>
#include <string>

namespace panel
{
namespace state
{
namespace manager
{

/** @class PanelStateManager
 *
 *  @brief State handler for the Op-Panel
 *
 *  Holds panel state information and tracks the current active functionality
 *  at which the panel is operating.
 */
class PanelStateManager
{
  public:
    /** @brief Deleted special members */
    PanelStateManager(const PanelStateManager&) = delete;
    PanelStateManager& operator=(const PanelStateManager&) = delete;
    PanelStateManager(PanelStateManager&&) = delete;
    PanelStateManager& operator=(PanelStateManager&&) = delete;

    ~PanelStateManager() = default;

    /**
     * @brief Parameterized constructor
     *
     * @param[in] transport - Transport object used to communicate with the
     * panel.
     *
     * @throws std::runtime_error
     */
    PanelStateManager(std::shared_ptr<Transport> transport) :
        transport(transport)
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
}; // class PanelStateManager

} // namespace manager
} // namespace state
} // namespace panel
