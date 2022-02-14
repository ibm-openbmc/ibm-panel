#pragma once

#include <string>
namespace panel
{
namespace tool
{

/**
 * @brief Api to handle input events.
 * This api receives input event and call dbus api to process button event
 * accordingly.
 * @param[in] input: Input event.
 *                   It can have values UP/DOWN or EXECUTE.
 */
void btnEventDbusCall(const std::string& input);

} // namespace tool
} // namespace panel
