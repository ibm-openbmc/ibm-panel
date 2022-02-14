#include "dbus_call.hpp"

#include "types.hpp"

#include <sdbusplus/bus.hpp>
#include <sdbusplus/exception.hpp>

namespace panel
{
namespace tool
{
void btnEventDbusCall(const std::string& input)
{
    auto param = -1;
    if ((input.compare("DOWN")) == 0)
    {
        param = types::ButtonEvent::DECREMENT;
    }
    else if ((input.compare("UP")) == 0)
    {
        param = types::ButtonEvent::INCREMENT;
    }
    else if ((input.compare("EXECUTE")) == 0)
    {
        param = types::ButtonEvent::EXECUTE;
    }
    else
    {
        throw std::runtime_error("Invalid Input");
    }

    auto bus = sdbusplus::bus::new_default_system();
    auto method = bus.new_method_call("com.ibm.PanelApp", "/com/ibm/panel_app",
                                      "com.ibm.panel", "ProcessButton");
    method.append(param);
    try
    {
        auto reply = bus.call(method);
    }
    catch (const sdbusplus::exception::SdBusError& e)
    {
        std::cerr << "SDBUS call failed: " << e;
        throw;
    }
}
} // namespace tool
} // namespace panel
