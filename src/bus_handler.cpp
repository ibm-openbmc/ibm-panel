#include "bus_handler.hpp"

#include <string>

#include "utils.cpp"

namespace panel
{
void BusHandler::display(const std::string& displayLine1,
                         const std::string& displayLine2)
{
    utils::sendCurrDisplayToPanel(displayLine1, displayLine2, transport);
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
    std::cout << "Bitmap received: ";
    for (const auto& item : functionBitMap)
    {
        std::cout << std::setfill('0') << std::setw(2) << std::hex << (int)item
                  << " ";
    }
    std::cout << std::endl;

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

    if (functionList.empty())
    {
        std::cout << "Empty function list received. Functions enabled before "
                     "will be disabled."
                  << std::endl;
    }

    // should be called even if the function list is empty, in case phyp wants
    // to disable all the functions.
    stateManager->toggleFuncStateFromPhyp(functionList);
}

void BusHandler::btnRequest(int event)
{
    switch (event)
    {
        case 0:
            stateManager->processPanelButtonEvent(
                types::ButtonEvent::INCREMENT);
            break;

        case 1:
            stateManager->processPanelButtonEvent(
                types::ButtonEvent::DECREMENT);
            break;

        case 2:
            stateManager->processPanelButtonEvent(types::ButtonEvent::EXECUTE);
            break;
        default:
            std::cerr << "Invalid Input" << std::endl;
    }
}
} // namespace panel
