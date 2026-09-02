#include "panel_state_manager.hpp"

#include <vector>

namespace panel
{
namespace state
{
namespace manager
{
enum StateType : types::Byte
{
    INITIAL_STATE = 0,
    DEBOUCNE_SRC_STATE = 125,
    STAR_STATE = 126,
    INVALID_STATE = 127,
};

enum SystemStateMask : types::FunctionMask
{
    NO_MASK = 0x00,
    ENABLE_BY_PHYP = 0x01,
    DISABLE_BY_PHYP = static_cast<SystemStateMask>(~ENABLE_BY_PHYP),
    ENABLE_BMC_STANDBY_STATE = 0x20,
    DISABLE_BMC_STANDBY_STATE =
        static_cast<SystemStateMask>(~ENABLE_BMC_STANDBY_STATE),
    ENABLE_POWER_STATE = 0x04,
    DISABLE_POWER_STATE = static_cast<SystemStateMask>(~ENABLE_POWER_STATE),
    ENABLE_PHYP_RUNTIME_STATE = 0x08,
    DISABLE_PHYP_RUNTIME_STATE =
        static_cast<SystemStateMask>(~ENABLE_PHYP_RUNTIME_STATE),
    ENABLE_CE_MODE = 0x10,
    DISABLE_CE_MODE = static_cast<SystemStateMask>(~ENABLE_CE_MODE),
    ENABLE_MANUAL_MODE = 0x02,
    DISABLE_MANUAL_MODE = static_cast<SystemStateMask>(~ENABLE_MANUAL_MODE),
    ENABLE_UNKNOWN_MODE = 0x40,
    DISABLE_UNKNOWN_MODE = static_cast<SystemStateMask>(~ENABLE_UNKNOWN_MODE),
    ENABLE_PASSIVE_MODE = 0x80,
    DISABLE_PASSIVE_MODE = static_cast<SystemStateMask>(~ENABLE_PASSIVE_MODE),
    ENABLE_ACTIVE_MODE = 0x0100,
    DISABLE_ACTIVE_MODE = static_cast<SystemStateMask>(~ENABLE_ACTIVE_MODE),
};

// structure defines functionality attributes.
struct FunctionalityAttributes
{
    types::FunctionNumber funcNumber;
    // Any function number not dependent on the state of the machine or any
    // other element will be enabled by default.
    bool defaultEnabled;
    bool isDebounceRequired;
    std::string debounceSrcValue;
    types::FunctionNumber subRangeEndPoint;

    // This will hold the conditions to enable that method.
    /* 0th bit - Enabled by Phyp.
     * 1st bit - Operation mode Normal/Manual 0/1
     * 2nd bit - Power on state Off/On 0/1
     * 3rd bit - Is Runtime No/Yes 0/1
     * 4th bit - CE No/Yes 0/1 - we need to discuss.
     * 5th bit - At standby No/Yes 0/1 - BMC state not ready to ready.
     * 6th bit - Unknown mode No/Yes 0/1 - BMC role is not determined.
     * 7th bit - Passive mode No/Yes 0/1 - BMC role is Passive.
     * 8th bit - Active mode No/Yes 0/1 - BMC role is Active.
     * 9-15 bits - Reserved
     */
    types::FunctionMask enableMask;
};

// This can be moved to a common file in case this information needs to be
// shared between files. List of functionalites, initialized to their
// default values.
std::vector<FunctionalityAttributes> functionalityList = {
    {1, true, false, "NONE", StateType::INITIAL_STATE,
     SystemStateMask::NO_MASK},
    {2, true, false, "NONE", StateType::INITIAL_STATE,
     SystemStateMask::NO_MASK},
    {3, false, true, "A1008003", StateType::INITIAL_STATE,
     (SystemStateMask::ENABLE_POWER_STATE |
      SystemStateMask::ENABLE_MANUAL_MODE |
      SystemStateMask::ENABLE_ACTIVE_MODE)},
    {4, true, false, "NONE", StateType::INITIAL_STATE,
     SystemStateMask::ENABLE_ACTIVE_MODE},
    {8, false, true, "A1008008", StateType::INITIAL_STATE,
     (SystemStateMask::ENABLE_POWER_STATE |
      SystemStateMask::ENABLE_MANUAL_MODE |
      SystemStateMask::ENABLE_ACTIVE_MODE)},
    {11, false, false, "NONE", StateType::INITIAL_STATE,
     SystemStateMask::NO_MASK},
    {12, false, false, "NONE", StateType::INITIAL_STATE,
     SystemStateMask::NO_MASK},
    {13, false, false, "NONE", StateType::INITIAL_STATE,
     SystemStateMask::NO_MASK},
    {14, false, false, "NONE", StateType::INITIAL_STATE,
     SystemStateMask::NO_MASK},
    {15, false, false, "NONE", StateType::INITIAL_STATE,
     SystemStateMask::NO_MASK},
    {16, false, false, "NONE", StateType::INITIAL_STATE,
     SystemStateMask::NO_MASK},
    {17, false, false, "NONE", StateType::INITIAL_STATE,
     SystemStateMask::NO_MASK},
    {18, false, false, "NONE", StateType::INITIAL_STATE,
     SystemStateMask::NO_MASK},
    {19, false, false, "NONE", StateType::INITIAL_STATE,
     SystemStateMask::NO_MASK},
    {20, true, false, "NONE", StateType::INITIAL_STATE,
     SystemStateMask::NO_MASK},
    {21, false, false, "NONE", StateType::INITIAL_STATE,
     (SystemStateMask::ENABLE_PHYP_RUNTIME_STATE |
      SystemStateMask::ENABLE_MANUAL_MODE | SystemStateMask::ENABLE_BY_PHYP |
      SystemStateMask::ENABLE_ACTIVE_MODE)},
    {22, false, true, "A1003022", StateType::INITIAL_STATE,
     (SystemStateMask::ENABLE_PHYP_RUNTIME_STATE |
      SystemStateMask::ENABLE_MANUAL_MODE | SystemStateMask::ENABLE_BY_PHYP |
      SystemStateMask::ENABLE_ACTIVE_MODE)},
    {25, true, false, "NONE", StateType::INITIAL_STATE,
     SystemStateMask::ENABLE_MANUAL_MODE},
    {26, true, false, "NONE", StateType::INITIAL_STATE,
     SystemStateMask::ENABLE_MANUAL_MODE},
    {30, false, false, "NONE", 0x01,
     (SystemStateMask::ENABLE_BMC_STANDBY_STATE |
      SystemStateMask::ENABLE_MANUAL_MODE)},
    {34, false, false, "NONE", StateType::INITIAL_STATE,
     (SystemStateMask::ENABLE_PHYP_RUNTIME_STATE |
      SystemStateMask::ENABLE_MANUAL_MODE | SystemStateMask::ENABLE_BY_PHYP |
      SystemStateMask::ENABLE_ACTIVE_MODE)},
    {41, false, true, "A1003041", StateType::INITIAL_STATE,
     (SystemStateMask::ENABLE_PHYP_RUNTIME_STATE |
      SystemStateMask::ENABLE_MANUAL_MODE | SystemStateMask::ENABLE_BY_PHYP |
      SystemStateMask::ENABLE_ACTIVE_MODE)},
    {42, false, true, "A1003042", StateType::INITIAL_STATE,
     (SystemStateMask::ENABLE_PHYP_RUNTIME_STATE |
      SystemStateMask::ENABLE_MANUAL_MODE |
      SystemStateMask::ENABLE_ACTIVE_MODE)},
    {43, true, true, "A1003043", StateType::INITIAL_STATE,
     SystemStateMask::ENABLE_MANUAL_MODE},
    {55, true, false, "NONE", 0x0D,
     SystemStateMask::ENABLE_MANUAL_MODE | SystemStateMask::ENABLE_CE_MODE |
         SystemStateMask::ENABLE_ACTIVE_MODE},
    {63, true, false, "NONE", 0x18,
     SystemStateMask::ENABLE_MANUAL_MODE | SystemStateMask::ENABLE_CE_MODE},
    {64, true, false, "NONE", 0x18,
     SystemStateMask::ENABLE_MANUAL_MODE | SystemStateMask::ENABLE_CE_MODE},
    {65, false, false, "NONE", StateType::INITIAL_STATE,
     (SystemStateMask::ENABLE_PHYP_RUNTIME_STATE |
      SystemStateMask::ENABLE_MANUAL_MODE | SystemStateMask::ENABLE_BY_PHYP |
      SystemStateMask::ENABLE_CE_MODE | SystemStateMask::ENABLE_ACTIVE_MODE)},
    {66, false, false, "NONE", StateType::INITIAL_STATE,
     (SystemStateMask::ENABLE_PHYP_RUNTIME_STATE |
      SystemStateMask::ENABLE_MANUAL_MODE | SystemStateMask::ENABLE_BY_PHYP |
      SystemStateMask::ENABLE_CE_MODE | SystemStateMask::ENABLE_ACTIVE_MODE)},
    {67, false, false, "NONE", StateType::INITIAL_STATE,
     (SystemStateMask::ENABLE_PHYP_RUNTIME_STATE |
      SystemStateMask::ENABLE_MANUAL_MODE | SystemStateMask::ENABLE_BY_PHYP |
      SystemStateMask::ENABLE_CE_MODE | SystemStateMask::ENABLE_ACTIVE_MODE)},
    {68, false, false, "NONE", StateType::INITIAL_STATE,
     (SystemStateMask::ENABLE_PHYP_RUNTIME_STATE |
      SystemStateMask::ENABLE_MANUAL_MODE | SystemStateMask::ENABLE_BY_PHYP |
      SystemStateMask::ENABLE_CE_MODE | SystemStateMask::ENABLE_ACTIVE_MODE)},
    {69, false, false, "NONE", StateType::INITIAL_STATE,
     (SystemStateMask::ENABLE_PHYP_RUNTIME_STATE |
      SystemStateMask::ENABLE_MANUAL_MODE | SystemStateMask::ENABLE_BY_PHYP |
      SystemStateMask::ENABLE_CE_MODE | SystemStateMask::ENABLE_ACTIVE_MODE)},
    {70, false, false, "NONE", StateType::INITIAL_STATE,
     (SystemStateMask::ENABLE_PHYP_RUNTIME_STATE |
      SystemStateMask::ENABLE_MANUAL_MODE | SystemStateMask::ENABLE_BY_PHYP |
      SystemStateMask::ENABLE_CE_MODE | SystemStateMask::ENABLE_ACTIVE_MODE)},
    {73, false, true, "A170800B", StateType::INITIAL_STATE,
     (SystemStateMask::ENABLE_MANUAL_MODE | SystemStateMask::ENABLE_CE_MODE |
      SystemStateMask::ENABLE_ACTIVE_MODE)},
    {74, false, false, "NONE", StateType::INITIAL_STATE,
     (SystemStateMask::ENABLE_MANUAL_MODE | SystemStateMask::ENABLE_CE_MODE)},
    {75, false, true, "A1003075", StateType::INITIAL_STATE,
     (SystemStateMask::ENABLE_CE_MODE | SystemStateMask::ENABLE_MANUAL_MODE)}};

void PanelStateManager::initPanelState() noexcept
{
    for (const auto& singleFunctionality : functionalityList)
    {
        PanelFunctionality aPanelFunctionality;
        aPanelFunctionality.functionNumber = singleFunctionality.funcNumber;
        aPanelFunctionality.functionActiveState =
            singleFunctionality.defaultEnabled;
        aPanelFunctionality.debouceSrc = singleFunctionality.debounceSrcValue;
        aPanelFunctionality.subFunctionUpperRange =
            singleFunctionality.subRangeEndPoint;
        aPanelFunctionality.functionEnableMask = singleFunctionality.enableMask;

        panelFunctions.push_back(aPanelFunctionality);
    }

    panelCurState = StateType::INITIAL_STATE;

    // Initailze panel Current substates
    panelCurSubStates.push_back(StateType::INITIAL_STATE);
    panelCurSubStates.push_back(StateType::INVALID_STATE);
    panelCurSubStates.push_back(StateType::INVALID_STATE);
}

} // namespace manager
} // namespace state
} // namespace panel
