#include "panel_state_manager.hpp"

#include "const.hpp"
#include "exception.hpp"
#include "utils.hpp"

#include <algorithm>
#include <xyz/openbmc_project/Common/error.hpp>

namespace panel
{
namespace state
{
namespace manager
{
enum StateType
{
    INITIAL_STATE = 0,
    DEBOUCNE_SRC_STATE = 125,
    STAR_STATE = 126,
    INVALID_STATE = 127,
};

enum SystemStateMask : uint8_t
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
};

static constexpr auto FUNCTION_02 = 2;
static constexpr auto FUNCTION_63 = 63;
static constexpr auto FUNCTION_64 = 64;

// structure defines fincttionaliy attributes.
struct FunctionalityAttributes
{
    types::FunctionNumber funcNumber;
    // Any function number not dependent on the state of the machine or any
    // other element will be enabled by default.
    bool defaultEnabled;
    bool isDebounceRequired;
    std::string debounceSrcValue;
    types::FunctionNumber subRangeEndPoint;

    // This will hold the conditions to ebnable that method.
    /* 0th bit - Enabled by Phyp.
     * 1st bit - Operation mode Normal/Manual 0/1
     * 2nd bit - Power on state Off/On 0/1
     * 3rd bit - Is Runtime No/Yes 0/1
     * 4th bit - CE No/Yes 0/1 - we need to discuss.
     * 5th bit - At standby No/Yes 0/1 - BMC state not ready to ready.
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
      SystemStateMask::ENABLE_MANUAL_MODE)},
    {4, true, false, "NONE", StateType::INITIAL_STATE,
     SystemStateMask::NO_MASK},
    {8, false, true, "A1008008", StateType::INITIAL_STATE,
     (SystemStateMask::ENABLE_POWER_STATE |
      SystemStateMask::ENABLE_MANUAL_MODE)},
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
      SystemStateMask::ENABLE_MANUAL_MODE | SystemStateMask::ENABLE_BY_PHYP)},
    {22, false, true, "A1003022", StateType::INITIAL_STATE,
     (SystemStateMask::ENABLE_PHYP_RUNTIME_STATE |
      SystemStateMask::ENABLE_MANUAL_MODE | SystemStateMask::ENABLE_BY_PHYP)},
    {25, true, false, "NONE", StateType::INITIAL_STATE,
     SystemStateMask::ENABLE_MANUAL_MODE},
    {26, true, false, "NONE", StateType::INITIAL_STATE,
     SystemStateMask::ENABLE_MANUAL_MODE},
    {30, false, false, "NONE", 0x01,
     (SystemStateMask::ENABLE_BMC_STANDBY_STATE |
      SystemStateMask::ENABLE_MANUAL_MODE)},
    {34, false, false, "NONE", StateType::INITIAL_STATE,
     (SystemStateMask::ENABLE_PHYP_RUNTIME_STATE |
      SystemStateMask::ENABLE_MANUAL_MODE | SystemStateMask::ENABLE_BY_PHYP)},
    {41, false, true, "A1003041", StateType::INITIAL_STATE,
     (SystemStateMask::ENABLE_PHYP_RUNTIME_STATE |
      SystemStateMask::ENABLE_MANUAL_MODE | SystemStateMask::ENABLE_BY_PHYP)},
    {42, false, true, "A1003042", StateType::INITIAL_STATE,
     (SystemStateMask::ENABLE_PHYP_RUNTIME_STATE |
      SystemStateMask::ENABLE_MANUAL_MODE)},
    {43, true, true, "A1003043", StateType::INITIAL_STATE,
     SystemStateMask::ENABLE_MANUAL_MODE},
    {55, true, false, "NONE", 0x0D,
     SystemStateMask::ENABLE_MANUAL_MODE | SystemStateMask::ENABLE_CE_MODE},
    {63, true, false, "NONE", 0x18,
     SystemStateMask::ENABLE_MANUAL_MODE | SystemStateMask::ENABLE_CE_MODE},
    {64, true, false, "NONE", 0x18,
     SystemStateMask::ENABLE_MANUAL_MODE | SystemStateMask::ENABLE_CE_MODE},
    {65, false, false, "NONE", StateType::INITIAL_STATE,
     (SystemStateMask::ENABLE_PHYP_RUNTIME_STATE |
      SystemStateMask::ENABLE_MANUAL_MODE | SystemStateMask::ENABLE_BY_PHYP |
      SystemStateMask::ENABLE_CE_MODE)},
    {66, false, false, "NONE", StateType::INITIAL_STATE,
     (SystemStateMask::ENABLE_PHYP_RUNTIME_STATE |
      SystemStateMask::ENABLE_MANUAL_MODE | SystemStateMask::ENABLE_BY_PHYP |
      SystemStateMask::ENABLE_CE_MODE)},
    {67, false, false, "NONE", StateType::INITIAL_STATE,
     (SystemStateMask::ENABLE_PHYP_RUNTIME_STATE |
      SystemStateMask::ENABLE_MANUAL_MODE | SystemStateMask::ENABLE_BY_PHYP |
      SystemStateMask::ENABLE_CE_MODE)},
    {68, false, false, "NONE", StateType::INITIAL_STATE,
     (SystemStateMask::ENABLE_PHYP_RUNTIME_STATE |
      SystemStateMask::ENABLE_MANUAL_MODE | SystemStateMask::ENABLE_BY_PHYP |
      SystemStateMask::ENABLE_CE_MODE)},
    {69, false, false, "NONE", StateType::INITIAL_STATE,
     (SystemStateMask::ENABLE_PHYP_RUNTIME_STATE |
      SystemStateMask::ENABLE_MANUAL_MODE | SystemStateMask::ENABLE_BY_PHYP |
      SystemStateMask::ENABLE_CE_MODE)},
    {70, false, false, "NONE", StateType::INITIAL_STATE,
     (SystemStateMask::ENABLE_PHYP_RUNTIME_STATE |
      SystemStateMask::ENABLE_MANUAL_MODE | SystemStateMask::ENABLE_BY_PHYP |
      SystemStateMask::ENABLE_CE_MODE)},
    {73, false, true, "A170800B", StateType::INITIAL_STATE,
     (SystemStateMask::ENABLE_MANUAL_MODE | SystemStateMask::ENABLE_CE_MODE)},
    {74, false, false, "NONE", StateType::INITIAL_STATE,
     (SystemStateMask::ENABLE_MANUAL_MODE | SystemStateMask::ENABLE_CE_MODE)},
    {75, false, true, "A1003075", StateType::INITIAL_STATE,
     (SystemStateMask::ENABLE_CE_MODE | SystemStateMask::ENABLE_MANUAL_MODE)}};

void PanelStateManager::enableFunctonality(
    const types::FunctionalityList& listOfFunctionalities)
{
    for (const auto& functionNumber : listOfFunctionalities)
    {
        auto pos =
            find_if(panelFunctions.begin(), panelFunctions.end(),
                    [functionNumber](const PanelFunctionality& afunctionality) {
                        return afunctionality.functionNumber == functionNumber;
                    });
        if (pos != panelFunctions.end())
        {
            // before enabling the function check if all the pre-conditions are
            // met.
            if ((pos->functionEnableMask == SystemStateMask::NO_MASK) ||
                (systemState | pos->functionEnabledByPhyp) ==
                    pos->functionEnableMask)
            {
                pos->functionActiveState = true;
            }
        }
        else
        {
            std::cout << "Entry for function Number " << functionNumber
                      << " not found" << std::endl;
        }
    }
}

void PanelStateManager::disableFunctonality(
    const panel::types::FunctionalityList& listOfFunctionalities)
{
    for (const auto& functionNumber : listOfFunctionalities)
    {
        auto pos =
            find_if(panelFunctions.begin(), panelFunctions.end(),
                    [functionNumber](const PanelFunctionality& afunctionality) {
                        return afunctionality.functionNumber == functionNumber;
                    });
        if (pos != panelFunctions.end())
        {
            if ((pos->functionEnableMask == SystemStateMask::NO_MASK) ||
                (systemState | pos->functionEnabledByPhyp) !=
                    pos->functionEnableMask)

            {
                pos->functionActiveState = false;
            }
        }
        else
        {
            std::cout << "Entry for functionality Number " << functionNumber
                      << " not found" << std::endl;
        }
    }
}

void PanelStateManager::toggleFuncStateFromPhyp(
    const types::FunctionalityList& list)
{
    // List contain function(s) which has been set as enabled by phyp. Rest all
    // other functions whose "functionEnabledByPhyp" flag is set as enabled
    // should be reset to disabled.
    for (auto& aFunction : panelFunctions)
    {
        auto pos = find_if(list.begin(), list.end(),
                           [&aFunction](const types::FunctionNumber funNum) {
                               return funNum == aFunction.functionNumber;
                           });

        if (pos != list.end())
        {
            // then the cout can be used to test the function number.
            std::cout << " Function enabled: " << (int)aFunction.functionNumber
                      << std::endl;

            aFunction.functionEnabledByPhyp = SystemStateMask::ENABLE_BY_PHYP;
        }
        else
        {
            if (aFunction.functionEnabledByPhyp ==
                SystemStateMask::ENABLE_BY_PHYP)
            {
                std::cout << " Function Disabled: "
                          << (int)aFunction.functionNumber << std::endl;

                // If the function was enabled and not in the list it should be
                // disabled.
                aFunction.functionEnabledByPhyp =
                    SystemStateMask::DISABLE_BY_PHYP;
            }
        }
    }
    updateFunctionStatus();
}

void PanelStateManager::printPanelStates()
{
    const PanelFunctionality& funcState = panelFunctions.at(panelCurState);
    std::cout << "Selected functionality = " << int(funcState.functionNumber)
              << std::endl;

    if (funcState.functionNumber == FUNCTION_02 && isSubrangeActive)
    {
        std::cout << "Active sub state level 0 = "
                  << functionality02[0].at(panelCurSubStates.at(0))
                  << std::endl;
        if (panelCurSubStates.at(1) != StateType::INVALID_STATE)
        {
            std::cout << "Active sub state level 1 = "
                      << functionality02[1].at(panelCurSubStates.at(1))
                      << std::endl;
            if (panelCurSubStates.at(2) != StateType::INVALID_STATE)
            {
                std::cout << "Active sub state level 2 = "
                          << functionality02[2].at(panelCurSubStates.at(2))
                          << std::endl;
            }
        }
    }
    else
    {
        if (isSubrangeActive)
        {
            if (panelCurSubStates.at(0) == StateType::INITIAL_STATE)
            {
                std::cout << "Active sub state level 0 = INITIAL" << std::endl;
            }
            else if (panelCurSubStates.at(0) == StateType::STAR_STATE)
            {
                std::cout << "Active sub state level 0 = **" << std::endl;
            }
            else
            {
                std::cout << "Current active sub state level 0 = "
                          << int(panelCurSubStates.at(0)) << std::endl;
            }
        }
    }
}

void PanelStateManager::processPanelButtonEvent(
    const types::ButtonEvent& button)
{
    // In case panel is in DEBOUCNE_SRC_STATE, and next button is increment
    // or decrement, it should come out of DEBOUCNE_SRC_STATE
    if (panelCurSubStates.at(0) == StateType::DEBOUCNE_SRC_STATE &&
        (button == types::ButtonEvent::INCREMENT ||
         button == types::ButtonEvent::DECREMENT))
    {
        panelCurSubStates.at(0) = StateType::INITIAL_STATE;
    }

    switch (button)
    {
        case types::ButtonEvent::INCREMENT:
            incrementState();
            break;

        case types::ButtonEvent::DECREMENT:
            decrementState();
            break;

        case types::ButtonEvent::EXECUTE:
            executeState();
            break;

        default:
            break;
    }

    std::cout << "Button event " << (int)button << " At function - "
              << (int)panelFunctions.at(panelCurState).functionNumber
              << " Panel cur state = " << (int)systemState << std::endl;

    // printPanelStates();
}

void PanelStateManager::resetStateManager()
{
    panelCurState = StateType::INITIAL_STATE;
    panelCurSubStates.push_back(StateType::INITIAL_STATE);
    panelCurSubStates.push_back(StateType::INVALID_STATE);
    panelCurSubStates.push_back(StateType::INVALID_STATE);
    funcExecutor->executeFunction(01, types::FunctionalityList{});
}

void PanelStateManager::initPanelState()
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
    panelCurSubStates.push_back(StateType::INITIAL_STATE);
    panelCurSubStates.push_back(StateType::INVALID_STATE);
    panelCurSubStates.push_back(StateType::INVALID_STATE);
}

std::tuple<types::FunctionNumber, types::FunctionNumber>
    PanelStateManager::getPanelCurrentStateInfo() const
{
    const PanelFunctionality& funcState = panelFunctions.at(panelCurState);
    return std::make_tuple(funcState.functionNumber, panelCurSubStates.at(0));
}

void PanelStateManager::initFunction02()
{
    try
    {
        auto sysValues = utils::readSystemParameters();

        if (std::get<0>(sysValues).empty() || std::get<1>(sysValues).empty())
        {
            throw std::runtime_error("Error reading system values");
        }

        utils::getNextBootSide(nextBootSideSelected);

        const auto& iplType = std::get<0>(sysValues);
        if (iplType == "A_Mode")
        {
            panelCurSubStates.at(0) = 0;
        }
        else if (iplType == "B_Mode")
        {
            panelCurSubStates.at(0) = 1;
        }
        else if (iplType == "C_Mode")
        {
            panelCurSubStates.at(0) = 2;
        }
        else if (iplType == "D_Mode")
        {
            panelCurSubStates.at(0) = 3;
        }
        else
        {
            // TODO: Add elog here to detect invalid mode.
            std::cout << "Invalid Mode" << std::endl;
        }

        const auto& systemOperatingMode = std::get<1>(sysValues);
        if (systemOperatingMode == "Manual")
        {
            panelCurSubStates.at(1) = 0;
        }
        else if (systemOperatingMode == "Normal")
        {
            panelCurSubStates.at(1) = 1;
        }

        if (nextBootSideSelected == "P")
        {
            panelCurSubStates.at(2) = 0;
        }
        else
        {
            panelCurSubStates.at(2) = 1;
        }
    }
    catch (const std::exception& e)
    {
        std::cout << e.what() << std::endl;
        // TODO: Display FF once that commit is in.
    }
}

// functionality 02
void PanelStateManager::setIPLParameters(const types::ButtonEvent& button)
{
    std::vector<std::string> subRange = functionality02.at(levelToOperate);
    static std::tuple<types::index, types::index, types::index> initialValues;

    switch (button)
    {
        case types::ButtonEvent::INCREMENT:
            if (panelCurSubStates.at(levelToOperate) == subRange.size() - 1)
            {
                panelCurSubStates.at(levelToOperate) = StateType::INITIAL_STATE;
            }
            else
            {
                panelCurSubStates.at(levelToOperate)++;
            }
            break;

        case types::ButtonEvent::DECREMENT:
            if (panelCurSubStates.at(levelToOperate) ==
                StateType::INITIAL_STATE)
            {
                panelCurSubStates.at(levelToOperate) = subRange.size() - 1;
            }
            else
            {
                panelCurSubStates.at(levelToOperate)--;
            }
            break;

        case types::ButtonEvent::EXECUTE:
            if (!isSubrangeActive)
            {
                isSubrangeActive = true;

                // set initial values
                initFunction02();
                initialValues = std::make_tuple(panelCurSubStates.at(0),
                                                panelCurSubStates.at(1),
                                                panelCurSubStates.at(2));
            }
            else if (levelToOperate != 2) // max depth of sub state
            {
                levelToOperate++;
            }
            else
            {
                // check if we need to toggle that parameter in write.
                // We need to change value only when different value has been
                // selected than the initial value.

                if (panelCurSubStates.at(0) == std::get<0>(initialValues))
                {
                    panelCurSubStates.at(0) = StateType::INVALID_STATE;
                }

                if (panelCurSubStates.at(1) == std::get<1>(initialValues))
                {
                    panelCurSubStates.at(1) = StateType::INVALID_STATE;
                }

                if (panelCurSubStates.at(2) == std::get<2>(initialValues))
                {
                    panelCurSubStates.at(2) = StateType::INVALID_STATE;
                }

                // if any one selected value is different than its initial
                // value, then only we need to execute.
                if (panelCurSubStates.at(0) != StateType::INVALID_STATE ||
                    panelCurSubStates.at(1) != StateType::INVALID_STATE ||
                    panelCurSubStates.at(2) != StateType::INVALID_STATE)
                {
                    try
                    {
                        funcExecutor->executeFunction(
                            panelFunctions.at(panelCurState).functionNumber,
                            panelCurSubStates);

                        if (panelCurSubStates.at(1) != StateType::INVALID_STATE)
                        {
                            if (panelCurSubStates.at(1) == 0)
                            {
                                std::cout << "Manual mode set" << std::endl;
                                setSystemOperatingMode("Manual");
                            }
                            else if (panelCurSubStates.at(1) == 1)
                            {
                                std::cout << "Normal mode set" << std::endl;
                                setSystemOperatingMode("Normal");
                            }
                        }
                    }
                    catch (const sdbusplus::exception::SdBusError& e)
                    {
                        std::cerr << "Error writing values to set system "
                                     "operating mode"
                                  << std::endl;
                    }

                    // reset all the flag
                    panelCurSubStates.at(0) = StateType::INITIAL_STATE;
                    panelCurSubStates.at(1) = StateType::INVALID_STATE;
                    panelCurSubStates.at(2) = StateType::INVALID_STATE;
                }

                levelToOperate = 0;
                isSubrangeActive = false;
            }
            break;

        default:
            break;
    }
    displayFunc02();
}

void PanelStateManager::incrementState()
{
    const PanelFunctionality& funcState = panelFunctions.at(panelCurState);

    if (funcState.functionNumber == FUNCTION_02 && isSubrangeActive)
    {
        setIPLParameters(types::ButtonEvent::INCREMENT);
        return;
    }

    // If sub range is active, it implies function has Sub functions.
    if (isSubrangeActive)
    {
        if (panelCurSubStates.at(0) == StateType::STAR_STATE)
        {
            // move to the intial method of the sub range which will be
            // always 00.
            panelCurSubStates.at(0) = StateType::INITIAL_STATE;
        }
        else if (panelCurSubStates.at(0) == funcState.subFunctionUpperRange)
        {
            // next substate should point to exiting the sub range
            panelCurSubStates.at(0) = StateType::STAR_STATE;
        }
        else
        {
            if (panelCurSubStates.at(0) < funcState.subFunctionUpperRange)
            {
                // get the next entry in subRange
                panelCurSubStates.at(0)++;
            }
        }
    }
    else
    {
        auto it = next(panelFunctions.begin(), (panelCurState + 1));
        for (; it != panelFunctions.end(); ++it)
        {
            if (it->functionActiveState == true)
            {
                panelCurState = distance(panelFunctions.begin(), it);
                break;
            }
        }

        // when we reach past last entry after traversing
        if (it == panelFunctions.end())
        {
            // reset the panelCurState
            auto pos = find_if(panelFunctions.begin(), panelFunctions.end(),
                               [](const PanelFunctionality& funcState) {
                                   if (funcState.functionActiveState)
                                   {
                                       return true;
                                   }
                                   return false;
                               });

            panelCurState = distance(panelFunctions.begin(), pos);
        }
    }
    createDisplayString();
}

void PanelStateManager::decrementState()
{
    const PanelFunctionality& funcState = panelFunctions.at(panelCurState);

    if (funcState.functionNumber == FUNCTION_02 && isSubrangeActive)
    {
        setIPLParameters(types::ButtonEvent::DECREMENT);
        return;
    }

    // If sub range is active it implies that function has sub-range.
    if (isSubrangeActive)
    {
        if (panelCurSubStates.at(0) == StateType::INITIAL_STATE)
        {
            panelCurSubStates.at(0) = StateType::STAR_STATE;
        }
        else if (panelCurSubStates.at(0) == StateType::STAR_STATE)
        {
            // roll back to the last method of the sub range
            panelCurSubStates.at(0) = funcState.subFunctionUpperRange;
        }
        else
        {
            if (panelCurSubStates.at(0) > StateType::INITIAL_STATE)
            {
                // get the next entry in subRange
                panelCurSubStates.at(0)--;
            }
        }
    }
    else
    {
        auto it = prev(panelFunctions.rend(), panelCurState);

        // decrement the panel current state till we get an enabled function
        for (; it != panelFunctions.rend(); ++it)
        {
            if (it->functionActiveState == true)
            {
                panelCurState = distance(it, (panelFunctions.rend() - 1));
                break;
            }
        }

        // when we reach to beginning while traversing
        if (it == panelFunctions.rend())
        {
            // reset the panelCurState
            auto nextpos =
                find_if(panelFunctions.rbegin(), panelFunctions.rend(),
                        [](const PanelFunctionality& funcState) {
                            if (funcState.functionActiveState)
                            {
                                return true;
                            }
                            return false;
                        });

            panelCurState = distance(nextpos, (panelFunctions.rend() - 1));
        }
    }
    createDisplayString();
}

void PanelStateManager::executeState()
{
    PanelFunctionality& funcState = panelFunctions.at(panelCurState);

    if (funcState.functionNumber == FUNCTION_02)
    {
        setIPLParameters(types::ButtonEvent::EXECUTE);
        return;
    }

    if (funcState.debouceSrc != "NONE" &&
        panelCurSubStates.at(0) != StateType::DEBOUCNE_SRC_STATE)
    {
        panelCurSubStates.at(0) = StateType::DEBOUCNE_SRC_STATE;
        displayDebounce();
        return;
    }

    // check if the current state has a subrange or function is 63 or 64. This
    // can be a situation for function 63 or 64 where upper range is equal to
    // StateType::INITIAL_STATE and still the function has sub range.
    // Sub functions of function 63 and 64 gets enabled at runtime based on
    // number of progress codes/SRC received respectively.
    // Below will be the case when number of Progress code or SRC received is 0.
    // In that case only 00 sub function needs to remain activated.
    if (funcState.subFunctionUpperRange != StateType::INITIAL_STATE ||
        funcState.functionNumber == FUNCTION_63 ||
        funcState.functionNumber == FUNCTION_64)
    {
        // Then check if it already active
        if (isSubrangeActive)
        {
            if (panelCurSubStates.at(0) == StateType::STAR_STATE)
            {
                // execute exit sub range protocol, Dont change the state,
                // just exit the sub loop
                isSubrangeActive = false;
                panelCurSubStates.at(0) = StateType::INITIAL_STATE;
                createDisplayString();
                std::cout << "Exit sub range, retain state at " << panelCurState
                          << std::endl;
            }
            else
            {
                std::cout << "Subrange is already active, execute the sub "
                             "functionality "
                          << panelCurSubStates.at(0) << " of functionality"
                          << panelCurState << std::endl;

                // after this execute do whatever is required to execute the
                // functionality
                funcExecutor->executeFunction(
                    panelFunctions.at(panelCurState).functionNumber,
                    panelCurSubStates);
            }
        }
        // if not active, activate it and point to star method
        else
        {
            if (funcState.functionNumber == FUNCTION_63 ||
                funcState.functionNumber == FUNCTION_64)
            {
                uint8_t count = 0;

                // If function is 63 then sub function is enabled based on
                // number of IPL SRCs.
                if (funcState.functionNumber == FUNCTION_63)
                {
                    count = funcExecutor->getIPLSRCCount();
                }

                // If function is 64 then sub function is enabled based on
                // number of Pel event received so far.
                if (funcState.functionNumber == FUNCTION_64)
                {
                    // fetch the existing list of PELs in the system, filter
                    // them and store their event Id for sub functions.
                    count = funcExecutor->getPelEventIdCount();
                }

                if (count != 0)
                {
                    //-1 to match the index
                    funcState.subFunctionUpperRange = count - 1;
                }
                else
                {
                    // since 0th sub function will always be enabled.
                    funcState.subFunctionUpperRange = count;
                }
            }

            isSubrangeActive = true;
            panelCurSubStates.at(0) = StateType::STAR_STATE;
            createDisplayString();

            std::cout << "Sub Range has been activated, execute the sub "
                         "functionality "
                      << panelCurSubStates.at(0) << " of functionality"
                      << panelCurState << std::endl;

            // after this execute do whatever is required to execute the
            // functionality
        }
    }
    else
    {
        // set this anyhow in case we are coming from debounce SRC state.
        panelCurSubStates.at(0) = StateType::INITIAL_STATE;

        if (panelFunctions.at(panelCurState).functionNumber == 25 ||
            panelFunctions.at(panelCurState).functionNumber == 26)
        {
            setCEState();
        }
        else
        {
            funcExecutor->executeFunction(
                panelFunctions.at(panelCurState).functionNumber,
                panelCurSubStates);
        }
    }

    // perform what ever operations needs to be done here, execute does not
    // need a state change
}

void PanelStateManager::createDisplayString() const
{
    std::string line1{};

    const auto& funcState = panelFunctions.at(panelCurState);

    std::stringstream ss;
    ss << std::setw(2) << std::setfill('0')
       << std::to_string(funcState.functionNumber);
    line1 += ss.str();

    if (isSubrangeActive)
    {
        if (panelCurSubStates.at(0) == StateType::STAR_STATE)
        {
            line1 += "**";
        }
        else
        {
            ss.str(std::string());
            ss << std::setw(2) << std::setfill('0') << std::hex
               << static_cast<int>(panelCurSubStates.at(0));
            line1 += ss.str();
        }
    }

    utils::sendCurrDisplayToPanel(line1, std::string{}, transport);
}

void PanelStateManager::displayDebounce() const
{
    std::string line2{};
    const auto& funcState = panelFunctions.at(panelCurState);

    // if function 3 or 8 then additional msg to be displayed along with
    // debounce.
    if (funcState.functionNumber == 3)
    {
        line2 = "REBOOT SERVER?";
    }
    else if (funcState.functionNumber == 8)
    {
        line2 = "SHUTDOWN SERVER?";
    }

    utils::sendCurrDisplayToPanel(funcState.debouceSrc, line2, transport);
}

/**
0 2 _ _ B_ _N __ __ _ _ _ _
_ _ _ _ __ __ __ __ T < _ _
*/
void PanelStateManager::displayFunc02()
{
    std::string line1(16, ' ');
    std::string line2(16, ' ');

    line1.replace(0, 2, "02");

    if (isSubrangeActive)
    {
        // If this property is set as false under panel interface,
        // implies that panel should not display OS IPL mode via function-02.
        if (funcExecutor->isOSIPLModeEnabled())
        {
            line1.replace(4, 1,
                          functionality02.at(0).at(panelCurSubStates.at(0)));
        }
        else
        {
            // Move the cursor to second level in case OS IPL mode is not
            // displayed.
            if (levelToOperate == 0)
            {
                levelToOperate++;
            }
        }

        line1.replace(7, 1, functionality02.at(1).at(panelCurSubStates.at(1)));
        line2.replace(12, 1, functionality02.at(2).at(panelCurSubStates.at(2)));

        if (levelToOperate == 0)
        {
            line1.replace(5, 1, 1, '<');
        }
        else if (levelToOperate == 1)
        {
            line1.replace(8, 1, 1, '<');
        }
        else if (levelToOperate == 2)
        {
            line2.replace(13, 1, 1, '<');
        }
    }
    utils::sendCurrDisplayToPanel(line1, line2, transport);
}

void PanelStateManager::updateFunctionStatus()
{
    for (auto& aFunction : panelFunctions)
    {
        if (aFunction.functionEnableMask != 0x00)
        {
            if (((systemState | aFunction.functionEnabledByPhyp) &
                 aFunction.functionEnableMask) == aFunction.functionEnableMask)
            {
                aFunction.functionActiveState = true;
            }
            else
            {
                aFunction.functionActiveState = false;
            }
        }
    }
}

void PanelStateManager::updateBMCState(const std::string& bmcState)
{
    // BMC state is anything other than NotReady
    if (bmcState != "xyz.openbmc_project.State.BMC.BMCState.NotReady")
    {
        // If BMC state is ready, execute functionality 01.
        if (bmcState == "xyz.openbmc_project.State.BMC.BMCState.Ready")
        {
            // Execute function 01 on bmc ready state.
            funcExecutor->executeFunction(1, types::FunctionalityList{});
        }

        // if the bit is not set
        if ((systemState & SystemStateMask::ENABLE_BMC_STANDBY_STATE) == 0x00)
        {
            // set the bit.
            systemState =
                systemState | SystemStateMask::ENABLE_BMC_STANDBY_STATE;
            updateFunctionStatus();
        }
    }
    // if the bit is already set and BMC state is NotReady
    else if ((bmcState == "xyz.openbmc_project.State.BMC.BMCState.NotReady") &&
             ((systemState & SystemStateMask::ENABLE_BMC_STANDBY_STATE) ==
              SystemStateMask::ENABLE_BMC_STANDBY_STATE))
    {
        // if the bit is set unset the bit
        systemState &= SystemStateMask::DISABLE_BMC_STANDBY_STATE;
        updateFunctionStatus();
    }
}

void PanelStateManager::updatePowerState(const std::string& powerState)
{
    if (powerState == "xyz.openbmc_project.State.Chassis.PowerState.On")
    {
        // if the bit is not set
        if ((systemState & SystemStateMask::ENABLE_POWER_STATE) ==
            SystemStateMask::NO_MASK)
        {
            // set the bit.
            systemState |= SystemStateMask::ENABLE_POWER_STATE;

            updateFunctionStatus();
        }
    }
    // if the bit is already set and state is off
    else if ((powerState ==
              "xyz.openbmc_project.State.Chassis.PowerState.Off") &&
             ((systemState & SystemStateMask::ENABLE_POWER_STATE) ==
              SystemStateMask::ENABLE_POWER_STATE))
    {
        // unset the bit
        systemState &= SystemStateMask::DISABLE_POWER_STATE;
        updateFunctionStatus();

        // As this property is not updated while power off currently, we need to
        // update progress code at poweroff to ASCII 0 so that HMC can clear
        // standby state.
        utils::writeBusProperty<
            std::tuple<std::vector<uint8_t>, std::vector<uint8_t>>>(
            "xyz.openbmc_project.State.Boot.Raw",
            "/xyz/openbmc_project/state/boot/raw0",
            "xyz.openbmc_project.State.Boot.Raw", "Value",
            std::make_tuple(constants::clearDisplayProgressCode,
                            std::vector<uint8_t>{0}));

        //  default function displayed when we power down.
        funcExecutor->executeFunction(1, types::FunctionalityList{});
    }
}

void PanelStateManager::updateBootProgressState(const std::string& bootState)
{
    // Phyp running.
    if (bootState ==
        "xyz.openbmc_project.State.Boot.Progress.ProgressStages.OSRunning")
    {
        // if the bit is not set
        if ((systemState & SystemStateMask::ENABLE_PHYP_RUNTIME_STATE) == 0x00)
        {
            // set the bit
            systemState |= SystemStateMask::ENABLE_PHYP_RUNTIME_STATE;

            updateFunctionStatus();
            return;
        }
    }
    else if ((systemState & SystemStateMask::ENABLE_PHYP_RUNTIME_STATE) ==
             SystemStateMask::ENABLE_PHYP_RUNTIME_STATE)
    {
        // unset the bit
        systemState &= SystemStateMask::DISABLE_PHYP_RUNTIME_STATE;
        updateFunctionStatus();
    }
}

void PanelStateManager::setSystemOperatingMode(const std::string& operatingMode)
{
    if (operatingMode == "Manual")
    {
        // Check if the bit is already not set
        if ((systemState & SystemStateMask::ENABLE_MANUAL_MODE) == 0x00)
        {
            // set the bit
            systemState |= SystemStateMask::ENABLE_MANUAL_MODE;
            updateFunctionStatus();
        }
    }
    // check if the bit is already unset
    else if ((systemState & SystemStateMask::ENABLE_MANUAL_MODE) != 0x00)
    {
        systemState &= SystemStateMask::DISABLE_MANUAL_MODE;
        updateFunctionStatus();
    }
}

// TODO: Need to verify how deactivate mode work. Can any of 25 or 26 executed
// when CE mode is active, deactivates CE mode or we need to follow a sequence?
void PanelStateManager::setCEState()
{
    // Irrespective of function number 25 or 26 if CE mode is already set,
    // disable it.
    if ((systemState & SystemStateMask::ENABLE_CE_MODE))
    {
        systemState &= SystemStateMask::DISABLE_CE_MODE;
        updateFunctionStatus();

        // When CE mode is disabled, reset the state.
        funcExecutor->executeFunction(
            panelFunctions.at(panelCurState).functionNumber, panelCurSubStates);
    }
    else
    {
        // if service switch 1 is enabled, means function 25 state is already
        // set. Subsequent calls to function 25 is neglected.
        if (funcExecutor->getServiceSwitch1State())
        {
            // if current event is for function 26
            if (panelFunctions.at(panelCurState).functionNumber == 26)
            {
                // set CE mode
                systemState |= SystemStateMask::ENABLE_CE_MODE;
                updateFunctionStatus();

                // call to executor is required to reset the state.
                funcExecutor->executeFunction(
                    panelFunctions.at(panelCurState).functionNumber,
                    panelCurSubStates);
            }
            else
            {
                // If this is subsequent call to function 25 just show 00.
                utils::sendCurrDisplayToPanel("25    00", std::string{},
                                              transport);
            }
        }
        else
        {
            // If service switch 1 is not set, call the executor irrespective of
            // function number to either set the state for function 25 or show
            // FF display for function 26.
            funcExecutor->executeFunction(
                panelFunctions.at(panelCurState).functionNumber,
                panelCurSubStates);
        }
    }
}

bool PanelStateManager::isFunctionSupported(const types::FunctionNumber funcNum)
{
    types::FunctionalityList supportedFuncs = {21, 22, 34, 65, 66,
                                               67, 68, 69, 70};

    if (find(supportedFuncs.begin(), supportedFuncs.end(), funcNum) !=
        supportedFuncs.end())
    {
        return true;
    }
    return false;
}

bool PanelStateManager::isRemoteAccessEnabled(
    const types::FunctionNumber funcNum)
{
    auto pos = find_if(panelFunctions.begin(), panelFunctions.end(),
                       [funcNum](const PanelFunctionality& afunctionality) {
                           return afunctionality.functionNumber == funcNum;
                       });
    if (pos != panelFunctions.end())
    {
        // if the function is enabled by PHYP and system at PHYP runtime.
        return (
            (pos->functionEnabledByPhyp == SystemStateMask::ENABLE_BY_PHYP) &&
            ((systemState & SystemStateMask::ENABLE_PHYP_RUNTIME_STATE) ==
             SystemStateMask::ENABLE_PHYP_RUNTIME_STATE));
    }

    return false;
}

types::ReturnStatus PanelStateManager::triggerFunctionDirectly(
    const types::FunctionNumber funcNum)
{
    if (isFunctionSupported(funcNum) && isRemoteAccessEnabled(funcNum))
    {
        return (funcExecutor->executeFunctionDirectly(funcNum));
    }

    std::cerr << "Function " << static_cast<int>(funcNum) << " is disabled."
              << std::endl;
    throw sdbusplus::xyz::openbmc_project::Common::Error::NotAllowed();
}

types::Binary PanelStateManager::getEnabledFunctionsList()
{
    types::Binary inputFunctions = {21, 22, 34, 65, 66, 67, 68, 69, 70};

    types::Binary enabledFunctions;
    enabledFunctions.reserve(inputFunctions.size());

    for (auto& val : inputFunctions)
    {
        if (isRemoteAccessEnabled(val))
        {
            enabledFunctions.push_back(val);
        }
    }
    return enabledFunctions;
}
} // namespace manager
} // namespace state
} // namespace panel
