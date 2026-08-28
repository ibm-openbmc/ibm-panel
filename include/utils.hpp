#pragma once

#include "types.hpp"

namespace panel
{
namespace utils
{

/**
 * @brief Create a Platform Event Log (PEL).
 *
 * @param[in] errIntf        - Error interface name.
 * @param[in] severity       - Severity of the event.
 * @param[in] additionalData - Key-value callout data attached to the PEL.
 */
void createPEL([[maybe_unused]] const std::string& errIntf,
               [[maybe_unused]] const std::string& severity,
               [[maybe_unused]] const types::PelAdditionalData& additionalData)
{
    // TODO: Implement PEL creation
}

} // namespace utils
} // namespace panel
