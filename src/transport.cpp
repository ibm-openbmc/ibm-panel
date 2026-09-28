#include "transport.hpp"

#include "const.hpp"

#include <phosphor-logging/lg2.hpp>

namespace panel
{

Transport::Transport() noexcept
{
}

Transport::~Transport() noexcept
{
}

void Transport::setTransportKey(bool keyValue)
{
    transportKey = keyValue;

    lg2::info("Transport Key is set to [{VALUE}]", "VALUE", keyValue);

    /**
     * ToDo:
     * When setting key to true
     * 1. Do Panel microcontroller Firmware update if required
     * 2. Reset the panel and do the configuration
     */
}
} // namespace panel
