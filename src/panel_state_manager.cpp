#include "panel_state_manager.hpp"

#include <algorithm>

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

static constexpr auto FUNCTION_02 = 2;

// structure defines fincttionaliy attributes.
struct FunctionalityAttributes
{
    panel::types::FunctionNumber funcNumber;
    // Any function number not dependent on the state of the machine or any
    // other element will be enabled by default.
    bool defaultEnabled;
    bool isDebounceRequired;
    std::string debounceSrcValue;
    panel::types::FunctionNumber subRangeEndPoint;
};

// This can be moved to a common file in case this information needs to be
// shared between files. List of functionalites, initialized to their
// default values.
std::vector<FunctionalityAttributes> functionalityList = {
    {1, true, false, "NONE", StateType::INITIAL_STATE},
    {2, true, false, "NONE", StateType::INITIAL_STATE},
    {3, false, true, "A1008003", StateType::INITIAL_STATE},
    {4, true, false, "NONE", StateType::INITIAL_STATE},
    {8, false, true, "A1008008", StateType::INITIAL_STATE},
    {11, false, false, "NONE", StateType::INITIAL_STATE},
    {12, false, false, "NONE", StateType::INITIAL_STATE},
    {13, false, false, "NONE", StateType::INITIAL_STATE},
    {14, false, false, "NONE", StateType::INITIAL_STATE},
    {15, false, false, "NONE", StateType::INITIAL_STATE},
    {16, false, false, "NONE", StateType::INITIAL_STATE},
    {17, false, false, "NONE", StateType::INITIAL_STATE},
    {18, false, false, "NONE", StateType::INITIAL_STATE},
    {19, false, false, "NONE", StateType::INITIAL_STATE},
    {20, true, false, "NONE", StateType::INITIAL_STATE},
    {21, false, false, "NONE", StateType::INITIAL_STATE},
    {22, false, true, "A1003022", StateType::INITIAL_STATE},
    {25, true, false, "NONE", StateType::INITIAL_STATE},
    {26, true, false, "NONE", StateType::INITIAL_STATE},
    {30, false, false, "NONE", 0x01},
    {34, false, false, "NONE", StateType::INITIAL_STATE},
    {41, false, true, "A1003041", StateType::INITIAL_STATE},
    {42, false, true, "A1003042", StateType::INITIAL_STATE},
    {43, true, true, "A1003043", StateType::INITIAL_STATE},
    {55, true, false, "NONE", 0x0D},
    {63, true, false, "NONE", 0x18},
    {64, true, false, "NONE", 0x18},
    {65, false, false, "NONE", StateType::INITIAL_STATE},
    {66, false, false, "NONE", StateType::INITIAL_STATE},
    {67, false, false, "NONE", StateType::INITIAL_STATE},
    {68, false, false, "NONE", StateType::INITIAL_STATE},
    {69, false, false, "NONE", StateType::INITIAL_STATE},
    {70, false, false, "NONE", StateType::INITIAL_STATE}};

PanelStateManager& PanelStateManager::getStateHandler()
{
    static PanelStateManager stateHandler;
    return stateHandler;
}

void PanelStateManager::enableFunctonality(
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
            pos->functionActiveState = true;
        }
        else
        {
            std::cout << "Entry for functionality Number " << functionNumber
                      << " not found" << std::endl;
        }
    }
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
    const panel::types::ButtonEvent& button)
{
    // In case panel is in DEBOUCNE_SRC_STATE, and next button is increment
    // or decrement, it should come out of DEBOUCNE_SRC_STATE
    if (panelCurSubStates.at(0) == StateType::DEBOUCNE_SRC_STATE &&
        (button == panel::types::ButtonEvent::INCREMENT ||
         button == panel::types::ButtonEvent::DECREMENT))
    {
        panelCurSubStates.at(0) = StateType::INITIAL_STATE;
    }

    switch (button)
    {
        case panel::types::ButtonEvent::INCREMENT:
            incrementState();
            break;

        case panel::types::ButtonEvent::DECREMENT:
            decrementState();
            break;

        case panel::types::ButtonEvent::EXECUTE:
            executeState();
            break;

        default:
            break;
    }

    // printStates();
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

        panelFunctions.push_back(aPanelFunctionality);
    }

    panelCurState = StateType::INITIAL_STATE;
    panelCurSubStates.push_back(StateType::INITIAL_STATE);
    panelCurSubStates.push_back(StateType::INVALID_STATE);
    panelCurSubStates.push_back(StateType::INVALID_STATE);
}

std::tuple<panel::types::FunctionNumber, panel::types::FunctionNumber>
    PanelStateManager::getPanelCurrentStateInfo() const
{
    const PanelFunctionality& funcState = panelFunctions.at(panelCurState);
    return std::make_tuple(funcState.functionNumber, panelCurSubStates.at(0));
}

// functionality 02
void PanelStateManager::setIPLParameters(
    const panel::types::ButtonEvent& button)
{
    // by default always increment level 0
    static uint8_t levelToOperate = 0;
    std::vector<std::string> subRange = functionality02.at(levelToOperate);

    switch (button)
    {
        case panel::types::ButtonEvent::INCREMENT:
            if (panelCurSubStates.at(levelToOperate) == subRange.size() - 1)
            {
                panelCurSubStates.at(levelToOperate) = StateType::INITIAL_STATE;
            }
            else
            {
                panelCurSubStates.at(levelToOperate)++;
            }
            break;

        case panel::types::ButtonEvent::DECREMENT:
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

        case panel::types::ButtonEvent::EXECUTE:
            if (!isSubrangeActive)
            {
                isSubrangeActive = true;

                // set sub states to initial value
                // TODO: implement a method to get inital values of substate.
                /* auto subStateInitialValue =
                     getInitialValue(this function does not exist, figure out);
                 lets say for test be it 2,1,1 index of functionality02 Map*/

                panelCurSubStates.at(0) = 2;
                panelCurSubStates.at(1) = 1;
                panelCurSubStates.at(2) = 1;
            }
            else if (levelToOperate != 2) // max depth of sub state
            {
                levelToOperate++;
            }
            else
            {
                // reset all the flag and execute as we are at last depth of
                // subsate and functionality needs to be executed.
                panelCurSubStates.at(0) = StateType::INITIAL_STATE;
                panelCurSubStates.at(1) = StateType::INVALID_STATE;
                panelCurSubStates.at(2) = StateType::INVALID_STATE;
                levelToOperate = 0;
                isSubrangeActive = false;
            }
            break;

        default:
            break;
    }
    createDisplayString(levelToOperate);
}

void PanelStateManager::incrementState()
{
    const PanelFunctionality& funcState = panelFunctions.at(panelCurState);

    if (funcState.functionNumber == FUNCTION_02 && isSubrangeActive)
    {
        setIPLParameters(panel::types::ButtonEvent::INCREMENT);
        return;
    }

    // check if current state has sub range and if it is active.
    if (funcState.subFunctionUpperRange != StateType::INITIAL_STATE &&
        isSubrangeActive)
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
}

void PanelStateManager::decrementState()
{
    const PanelFunctionality& funcState = panelFunctions.at(panelCurState);

    if (funcState.functionNumber == FUNCTION_02 && isSubrangeActive)
    {
        setIPLParameters(panel::types::ButtonEvent::DECREMENT);
        return;
    }

    // if the functionality has sub range and is in active state, loop in
    // sub range
    if (funcState.subFunctionUpperRange != StateType::INITIAL_STATE &&
        isSubrangeActive)
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
}

void PanelStateManager::executeState()
{
    const PanelFunctionality& funcState = panelFunctions.at(panelCurState);

    if (funcState.functionNumber == FUNCTION_02)
    {
        setIPLParameters(panel::types::ButtonEvent::EXECUTE);
        return;
    }

    if (funcState.debouceSrc != "NONE" &&
        panelCurSubStates.at(0) != StateType::DEBOUCNE_SRC_STATE)
    {
        panelCurSubStates.at(0) = StateType::DEBOUCNE_SRC_STATE;
        // TODO: Send this SRC to display
        std::cout << "Send this SRC to display " << funcState.debouceSrc
                  << std::endl;
        return;
    }

    // check if the current state has a subrange.
    if (funcState.subFunctionUpperRange != StateType::INITIAL_STATE)
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
            }
        }
        // if not active, activate it and point to star method
        else
        {
            isSubrangeActive = true;
            panelCurSubStates.at(0) = StateType::STAR_STATE;

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
        std::cout << "Execute method Directly" << std::endl;
    }

    // perform what ever operations needs to be done here, execute does not
    // need a state change
}

void PanelStateManager::createDisplayString(uint8_t level)
{
    std::ignore = level;
    /*check for issubstate active to conclude if substate display is at all
     * required*/
    /*Define here*/
}

} // namespace manager
} // namespace state
} // namespace panel
