#include "bus_handler.hpp"

#include <string>

#include "utils.cpp"

namespace panel
{
void BusHandler::display(const std::string& displayLine1,
                         const std::string& displayLine2)
{
    // added cout to escape from unused variable error. can be removed once the
    // implementation is added.
    std::cout << displayLine1 << displayLine2 << std::endl;
    // Implement display function
}

void BusHandler::triggerPanelLampTest(const bool state)
{
    if (state)
    {
        utils::doLampTest(transport);
    }
    else
    {
        utils::restoreDisplayOnPanel(transport);
    }
}

void BusHandler::toggleFunctionState(types::FunctionalityList functionBitMap)
{
    types::FunctionalityList functionList;

    for (types::Byte bitIndex = 1; bitIndex < functionBitMap.size() * 8;
         bitIndex++)
    {
        types::Byte byteIndex = bitIndex / 8;
        types::Byte bitIndexInsideByte = bitIndex % 8;

        if (functionBitMap[byteIndex] & (1 << bitIndexInsideByte))
        {
            functionList.push_back((byteIndex * 8) + bitIndexInsideByte);
        }
    }

    // should be called even if the function list is empty, in case phyp wants
    // to disable all the functions.
    stateManager->toggleFuncStateFromPhyp(functionList);
}
} // namespace panel
