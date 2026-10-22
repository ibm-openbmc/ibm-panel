#include "transport.hpp"

#include "const.hpp"

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

    /**
     * ToDo:
     * When setting key to true
     * 1. Do Panel microcontroller Firmware update if required
     * 2. Reset the panel and do the configuration
     */
}
} // namespace panel
