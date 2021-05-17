#include "panel_state_manager.hpp"

#include <assert.h>

#include <algorithm>
#include <fstream>
#include <sstream>
#include <tuple>

namespace panel
{
namespace state
{
namespace manager
{

using namespace std;
using namespace types;

static constexpr auto SUB_RANGE_INITIAL_STATE = 0;

// This is a state at which the functionality needs to exit sub range looping,
// mimics start method basically.
static constexpr auto SUB_RANGE_STAR_STATE = 255;

// structure defines fincttionaliy attributes.
struct FunctionalityAttributes
{
    FunctionNumber funcNumber;
    // Any function number not dependent on the state of the machine or any
    // other element will be enabled by default.
    bool defaultEnabled;
    bool isDebounceRequired;
    string debounceSrcValue;
    FunctionNumber subRangeEndPoint;
};

// This can be moved to a common file in case this information needs to be
// shared between files. List of functionalites, initialized to their default
// values.
std::vector<FunctionalityAttributes> functionalityList = {
    {1, true, false, "NONE", SUB_RANGE_INITIAL_STATE},
    {2, true, false, "NONE", SUB_RANGE_INITIAL_STATE},
    {3, false, true, "A1008003", SUB_RANGE_INITIAL_STATE},
    {4, true, false, "NONE", SUB_RANGE_INITIAL_STATE},
    {8, false, true, "A1008008", SUB_RANGE_INITIAL_STATE},
    {11, false, false, "NONE", SUB_RANGE_INITIAL_STATE},
    {12, false, false, "NONE", SUB_RANGE_INITIAL_STATE},
    {13, false, false, "NONE", SUB_RANGE_INITIAL_STATE},
    {14, false, false, "NONE", SUB_RANGE_INITIAL_STATE},
    {15, false, false, "NONE", SUB_RANGE_INITIAL_STATE},
    {16, false, false, "NONE", SUB_RANGE_INITIAL_STATE},
    {17, false, false, "NONE", SUB_RANGE_INITIAL_STATE},
    {18, false, false, "NONE", SUB_RANGE_INITIAL_STATE},
    {19, false, false, "NONE", SUB_RANGE_INITIAL_STATE},
    {20, true, false, "NONE", SUB_RANGE_INITIAL_STATE},
    {21, false, false, "NONE", SUB_RANGE_INITIAL_STATE},
    {22, false, true, "A1003022", SUB_RANGE_INITIAL_STATE},
    {25, true, false, "NONE", SUB_RANGE_INITIAL_STATE},
    {26, true, false, "NONE", SUB_RANGE_INITIAL_STATE},
    {30, false, false, "NONE", 0x03},
    {34, false, false, "NONE", SUB_RANGE_INITIAL_STATE},
    {41, false, true, "A1003041", SUB_RANGE_INITIAL_STATE},
    {42, false, true, "A1003042", SUB_RANGE_INITIAL_STATE},
    {43, true, true, "A1003043", SUB_RANGE_INITIAL_STATE},
    {55, true, false, "NONE", 0x0D},
    {63, true, false, "NONE", 0x18},
    {64, true, false, "NONE", 0x18},
    {65, false, false, "NONE", SUB_RANGE_INITIAL_STATE},
    {66, false, false, "NONE", SUB_RANGE_INITIAL_STATE},
    {67, false, false, "NONE", SUB_RANGE_INITIAL_STATE},
    {68, false, false, "NONE", SUB_RANGE_INITIAL_STATE},
    {69, false, false, "NONE", SUB_RANGE_INITIAL_STATE},
    {70, false, false, "NONE", SUB_RANGE_INITIAL_STATE}};

PanelStateManager& PanelStateManager::getStateHandler()
{
    static PanelStateManager stateHandler;
    return stateHandler;
}

void PanelStateManager::enableFunctonality(
    const FunctionalityList& listOfFunctionalities)
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
            cout << "Entry for functionality Number " << functionNumber
                 << " not found" << std::endl;
        }
    }
}

void PanelStateManager::processPanelButtonEvent(const ButtonEvent& button)
{
    switch (button)
    {
        case INCREMENT:
            incrementState();
            break;

        case DECREMENT:
            decrementState();
            break;

        case EXECUTE:
            executeState();
            break;
    }
}

void PanelStateManager::initPanelState()
{
    for (const auto& singleFunctionality : functionalityList)
    {
        PanelFunctionality aPanelFunctionality;
        aPanelFunctionality.functionNumber = singleFunctionality.funcNumber;
        aPanelFunctionality.functionActiveState =
            singleFunctionality.defaultEnabled;
        aPanelFunctionality.isDebounceRequired =
            singleFunctionality.isDebounceRequired;
        aPanelFunctionality.subFunctionUpperRange =
            singleFunctionality.subRangeEndPoint;

        panelFunctions.push_back(aPanelFunctionality);
    }

    // initialize state to first enabled method
    auto pos = find_if(panelFunctions.begin(), panelFunctions.end(),
                       [](const PanelFunctionality& aFunctionality) {
                           if (aFunctionality.functionActiveState)
                           {
                               return true;
                           }
                           return false;
                       });

    if (pos != panelFunctions.end())
    {
        panelCurState = distance(panelFunctions.begin(), pos);
        panelCurSubState = SUB_RANGE_INITIAL_STATE;
    }
    else
    {
        assert("No Enabled method for Op-Panel");
    }
}

std::tuple<FunctionNumber, FunctionNumber>
    PanelStateManager::getPanelCurrentStateInfo() const
{
    const PanelFunctionality& funcState = panelFunctions.at(panelCurState);
    return std::make_tuple(funcState.functionNumber, panelCurSubState);
}

void PanelStateManager::incrementState()
{
    const PanelFunctionality& funcState = panelFunctions.at(panelCurState);

    // check if current state has sub range and if it is active.
    if (funcState.subFunctionUpperRange != SUB_RANGE_INITIAL_STATE &&
        isSubrangeActive)
    {
        if (panelCurSubState == SUB_RANGE_STAR_STATE)
        {
            // move to the intial method of the sub range which will be
            // always 00.
            panelCurSubState = SUB_RANGE_INITIAL_STATE;
        }
        else if (panelCurSubState == funcState.subFunctionUpperRange)
        {
            // next substate should point to exiting the sub range
            panelCurSubState = SUB_RANGE_STAR_STATE;
        }
        else
        {
            if (panelCurSubState < funcState.subFunctionUpperRange)
            {
                // get the next entry in subRange
                panelCurSubState++;
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

    // if the functionality has sub range and is in active state, loop in sub
    // range
    if (funcState.subFunctionUpperRange != SUB_RANGE_INITIAL_STATE &&
        isSubrangeActive)
    {
        if (panelCurSubState == SUB_RANGE_INITIAL_STATE)
        {
            panelCurSubState = SUB_RANGE_STAR_STATE;
        }
        else if (panelCurSubState == SUB_RANGE_STAR_STATE)
        {
            // roll back to the last method of the sub range
            panelCurSubState = funcState.subFunctionUpperRange;
        }
        else
        {
            if (panelCurSubState > SUB_RANGE_INITIAL_STATE)
            {
                // get the next entry in subRange
                panelCurSubState--;
            }
        }
    }
    else
    {
        auto it = make_reverse_iterator(
            next(panelFunctions.begin(), (panelCurState - 1)));
        // decrement the panel current state till we get an enabled function
        for (; it != panelFunctions.rend(); ++it)
        {
            if (it->functionActiveState == true)
            {
                panelCurState = distance(it, panelFunctions.rend());
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

            panelCurState = distance(nextpos, panelFunctions.rend());
        }
    }
}

void PanelStateManager::executeState()
{
    const PanelFunctionality& funcState = panelFunctions.at(panelCurState);

    // check if the current state has a subrange.
    if (funcState.subFunctionUpperRange != SUB_RANGE_INITIAL_STATE)
    {
        // Then check if it already active
        if (isSubrangeActive)
        {
            if (panelCurSubState == SUB_RANGE_STAR_STATE)
            {
                // execute exit sub range protocol, Dont change the state, just
                // exit the sub loop
                isSubrangeActive = false;
                panelCurSubState = SUB_RANGE_INITIAL_STATE;

                std::cout << "Exit sub range, retain state at " << panelCurState
                          << std::endl;
            }
            else
            {
                std::cout << "Subrange is already active, execute the sub "
                             "functionality "
                          << panelCurSubState << " of functionality"
                          << panelCurState << std::endl;

                // after this execute do whatever is required to execute the
                // functionality
            }
        }
        // if not active, activate it and point to star method
        else
        {
            isSubrangeActive = true;
            panelCurSubState = SUB_RANGE_STAR_STATE;

            std::cout << "Sub Range has been activated, execute the sub "
                         "functionality "
                      << panelCurSubState << " of functionality"
                      << panelCurState << std::endl;

            // after this execute do whatever is required to execute the
            // functionality
        }
    }
    else
    {
        std::cout << "No Sub range just execute the current state method "
                  << panelCurState << std::endl;
    }

    // perform what ever operations needs to be done here, execute does not need
    // a state change
}

} // namespace manager
} // namespace state
} // namespace panel
