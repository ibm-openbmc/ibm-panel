#include "transport.hpp"

#include "const.hpp"

namespace panel
{

Transport::Transport() noexcept : devPath(" "), devAddress(0)
{
    // Default constructor – used for testing / stub instantiation.
    // No device path or address is set; transport key remains false.
}

Transport::Transport(const std::string& devPath, const uint8_t& devAddr,
                     const std::string& objectPath) :
    devPath(devPath), devAddress(devAddr), fruPath(objectPath)
{
    // TODO: Invoke panelI2CSetup() to open the I2C device file and
    // bind to the slave address.
}

Transport::~Transport() noexcept
{
    // TODO: Close panelFileDescriptor if it holds a valid fd.
}

void Transport::panelI2CSetup()
{
    // TODO: Establish the I2C connection to the panel.
    //  1. Validate devPath and devAddress are non-empty / non-zero.
    //  2. open() the device node at devPath.
    //     On failure log a PEL and throw .
    //  3. ioctl(fd, I2C_SLAVE, devAddress) to bind to the slave address.
    //     On failure log a PEL and throw.
    //  4. Store the formatted hex address in i2cAddress for use in PELs.
}

void Transport::panelI2CWrite(
    [[maybe_unused]] const types::Binary& buffer) const noexcept
{
    // TODO: Write buffer bytes to the panel over I2C.
    //  - Guard on transportKey; skip silently if key is false.
    //  - Guard on non-empty buffer.
    //  - Attempt write() up to max retry times;
    //  - After exhausting retries log a PEL (deviceWriteFailure).
}

void Transport::setTransportKey([[maybe_unused]] bool keyValue) noexcept
{
    // TODO: Flip transportKey to keyValue.
    //  - Log the new key value and device details for diagnostics.
}

} // namespace panel
