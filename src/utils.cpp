#include "utils.hpp"

#include "const.hpp"

#include <phosphor-logging/lg2.hpp>
#include <sdbusplus/bus.hpp>
#include <sdbusplus/exception.hpp>

namespace panel
{
namespace utils
{

void createPEL(const std::string& errIntf, const std::string& severity,
               const types::PelAdditionalData& additionalData)
{
    try
    {
        auto bus = sdbusplus::bus::new_default();
        auto method =
            bus.new_method_call(constants::eventLoggingServiceName,
                                constants::eventLoggingObjectPath,
                                constants::eventLoggingInterface, "Create");

        method.append(errIntf, severity, additionalData);
        bus.call(method);
    }
    catch (const sdbusplus::exception_t& ex)
    {
        lg2::error("PEL creation failed with an error: {ERROR}", "ERROR", ex);
    }
}

} // namespace utils
} // namespace panel
