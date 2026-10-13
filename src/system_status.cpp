#include "system_status.hpp"

namespace panel
{
void SystemStatus::initialize() noexcept
{
    /**
     * 1. Iterate through config JSON, create dbus listeners for the following
     *   1.1 Create NameOwner changed match for each Service
     *   1.2 Check is service is running already
     *   1.3 Create PropertChanged match for each object path and interface
     *   1.4 If service is already running, read property value and update value
     * in structure member
     * 2. Pass updated structure member to StateManager, to handle the current
     * system status.
     */
}
} // namespace panel