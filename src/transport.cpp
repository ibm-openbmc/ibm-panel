#include "transport.hpp"

#include "i2c_message_encoder.hpp"
#include "utils.hpp"

#include <fcntl.h>
#include <linux/i2c-dev.h>
#include <sys/ioctl.h>

#include <chrono>
#include <cstring>
#include <sstream>
#include <thread>

namespace panel
{
void Transport::panelI2CSetup()
{
    std::ostringstream i2cAddress;
    i2cAddress << "0x" << std::hex << std::uppercase << devAddress;
    if ((panelFileDescriptor = open(devPath.data(), O_WRONLY | O_NONBLOCK)) ==
        -1) // open failure
    {
        auto err = errno;
        std::string error = "Failed to open device file. Errno : ";
        error += std::to_string(err);
        error += "Errno description : ";
        error += strerror(err);
        error += " for device path ";
        error += devPath;
        std::map<std::string, std::string> additionData{};
        additionData.emplace("DESCRIPTION", error);
        additionData.emplace("CALLOUT_IIC_BUS", devPath);
        additionData.emplace("CALLOUT_IIC_ADDR", i2cAddress.str());
        panel::utils::createPEL(
            "com.ibm.Panel.Error.I2CSetupFailure",
            "xyz.openbmc_project.Logging.Entry.Level.Warning", additionData);
        throw std::runtime_error(error);
    }
    if (ioctl(panelFileDescriptor, I2C_SLAVE, devAddress) ==
        -1) // access failure
    {
        auto err = errno;
        std::string error = "Failed to access device path. <";
        error += devPath;
        error += "> at device address <0x";
        error += devAddress;
        error += ">. Errno : ";
        error += std::to_string(err);
        error += ". Errno description : ";
        error += strerror(err);
        std::map<std::string, std::string> additionData{};
        additionData.emplace("DESCRIPTION", error);
        additionData.emplace("CALLOUT_IIC_BUS", devPath);
        additionData.emplace("CALLOUT_IIC_ADDR", i2cAddress.str());
        panel::utils::createPEL(
            "com.ibm.Panel.Error.I2CSetupFailure",
            "xyz.openbmc_project.Logging.Entry.Level.Warning", additionData);
        throw std::runtime_error(error);
    }
    std::cout << "Success opening and accessing the device path: " << devPath
              << std::endl;
}

void Transport::panelI2CWrite(const types::Binary& buffer) const
{
    std::ostringstream i2cAddress;
    i2cAddress << "0x" << std::hex << std::uppercase << devAddress;
    if (transportKey)
    {
        if (buffer.size()) // check if the given buffer has data in it.
        {
            static constexpr auto maxRetry = 5; // Just a random value
            for (auto retryLoop = 0; retryLoop < maxRetry; ++retryLoop)
            {
                auto returnedSize =
                    write(panelFileDescriptor, buffer.data(), buffer.size());
                if (returnedSize !=
                    static_cast<int>(buffer.size())) // write failure
                {
                    std::cerr << "\n I2C Write failure. Errno : " << errno
                              << ". Errno description : " << strerror(errno)
                              << ". Bytes written = " << returnedSize
                              << ". Actual Bytes = " << buffer.size()
                              << ". Retry = " << retryLoop << std::endl;
                    std::map<std::string, std::string> additionData{};
                    additionData.emplace("DESCRIPTION", strerror(errno));
                    additionData.emplace("CALLOUT_IIC_BUS", devPath);
                    additionData.emplace("CALLOUT_IIC_ADDR", i2cAddress.str());
                    additionData.emplace("CALLOUT_ERRNO",
                                         std::to_string(errno));
                    panel::utils::createPEL(
                        "xyz.openbmc_project.Common.Device.Error.WriteFailure",
                        "xyz.openbmc_project.Logging.Entry.Level.Warning",
                        additionData);
                    continue;
                }
                break;
            }
        }
        else
        {
            std::cerr << "\n Buffer empty. Skipping I2C Write." << std::endl;
        }
    }
}

void Transport::doButtonConfig()
{
    encoder::MessageEncoder encode;
    panelI2CWrite(encode.buttonControl(0x00, 0x01));
    panelI2CWrite(encode.buttonControl(0x01, 0x01));
    panelI2CWrite(encode.buttonControl(0x02, 0x01));
    std::cout << "\n Button configuration done." << std::endl;
}

void Transport::doSoftReset()
{
    using namespace std::chrono_literals;
    panelI2CWrite(encoder::MessageEncoder().softReset());
    std::this_thread::sleep_for(3000ms);
    std::cout << "\n Panel:Soft reset done." << std::endl;
}

void Transport::setTransportKey(bool keyValue)
{
    if (!transportKey && keyValue && panelType == types::PanelType::LCD)
    {
        transportKey = keyValue;
        doSoftReset();
        doButtonConfig();
    }
    else
    {
        transportKey = keyValue;
    }
    std::cout << "\nTransport key is set to " << transportKey
              << " for the panel at " << devPath << ", " << devAddress
              << std::endl;
}

} // namespace panel
