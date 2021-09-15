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

} // namespace panel