#include "transport.hpp"

#include "const.hpp"
#include "utils.hpp"

#include <fcntl.h>
#include <linux/i2c-dev.h>
#include <sys/ioctl.h>

#include <cstring>
#include <format>
#include <phosphor-logging/lg2.hpp>

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
    panelI2CSetup();
}

Transport::~Transport() noexcept
{
    if (panelFileDescriptor != -1)
    {
        close(panelFileDescriptor);
    }
}

void Transport::panelI2CSetup()
{
    // Convert to hex address formatting
    i2cAddress = std::format("0x{:02X}", devAddress);

    if ((panelFileDescriptor = open(devPath.data(), O_RDWR | O_NONBLOCK)) == -1)
    {
        auto err = errno;
        std::string error =
            std::format("Failed to open device file. Errno : {}. "
                        "Errno description : {} for device path {}",
                        err, strerror(err), devPath);

        types::PelAdditionalData additionData{
            {"DESCRIPTION", error},
            {"CALLOUT_IIC_BUS", devPath},
            {"CALLOUT_IIC_ADDR", i2cAddress},
            {"CALLOUT_ERRNO", std::to_string(err)}};

        panel::utils::createPEL(
            "com.ibm.Panel.Error.I2CSetupFailure",
            "xyz.openbmc_project.Logging.Entry.Level.Warning", additionData);

        throw std::runtime_error(error);
    }

    if (ioctl(panelFileDescriptor, I2C_SLAVE, devAddress) == -1)
    {
        auto err = errno;
        std::string error =
            std::format("Failed to access device path. <{}> at device address"
                        " <{}>. Errno : {}. Errno description : {}",
                        devPath, i2cAddress, err, strerror(err));

        types::PelAdditionalData additionData{
            {"DESCRIPTION", error},
            {"CALLOUT_IIC_BUS", devPath},
            {"CALLOUT_IIC_ADDR", i2cAddress},
            {"CALLOUT_ERRNO", std::to_string(err)}};

        panel::utils::createPEL(
            "com.ibm.Panel.Error.I2CSetupFailure",
            "xyz.openbmc_project.Logging.Entry.Level.Warning", additionData);

        throw std::runtime_error(error);
    }

    lg2::info("Success opening and accessing the device path: {PATH}", "PATH",
              devPath);
}

void Transport::panelI2CWrite(const types::Binary& buffer) const noexcept
{
    if (transportKey)
    {
        if (!buffer.empty())
        {
            ssize_t returnedSize = 0;
            int retriesDone = 0;
            bool writeFailed = false;
            int failedErrno = 0;

            for (auto retryLoop = 0;
                 retryLoop < panel::constants::maxRetryCount; ++retryLoop)
            {
                retriesDone = retryLoop;
                writeFailed = false;
                returnedSize =
                    write(panelFileDescriptor, buffer.data(), buffer.size());

                if (returnedSize != static_cast<int>(buffer.size()))
                {
                    writeFailed = true;
                    failedErrno = errno;
                    sleep(1);
                    continue;
                }
                break;
            }

            if (writeFailed)
            {
                lg2::error(
                    "I2C Write failure. Errno:{ERRNO}, Description:{DESC}, "
                    "Bytes written:{WRITTEN}, Actual bytes:{ACTUAL}, "
                    "Retries:{RETRY}",
                    "ERRNO", failedErrno, "DESC", strerror(failedErrno),
                    "WRITTEN", returnedSize, "ACTUAL", buffer.size(), "RETRY",
                    retriesDone);

                types::PelAdditionalData additionData{
                    {"DESCRIPTION", strerror(failedErrno)},
                    {"CALLOUT_IIC_BUS", devPath},
                    {"CALLOUT_IIC_ADDR", i2cAddress},
                    {"CALLOUT_ERRNO", std::to_string(failedErrno)}};

                panel::utils::createPEL(
                    constants::deviceWriteFailure,
                    "xyz.openbmc_project.Logging.Entry.Level.Warning",
                    additionData);
            }
        }
        else
        {
            lg2::warning("Buffer empty. Skipping I2C Write.");
        }
    }
}

void Transport::setTransportKey(bool keyValue) noexcept
{
    transportKey = keyValue;

    lg2::info("Transport key set to {KEY} for panel at {PATH}, {ADDR}", "KEY",
              transportKey, "PATH", devPath, "ADDR", i2cAddress);
}

} // namespace panel
