#include "bus_handler.hpp"

#include <string>

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
} // namespace panel
