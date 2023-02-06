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
    if ((panelFileDescriptor = open(devPath.data(), O_RDWR | O_NONBLOCK)) ==
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
            ssize_t returnedSize = 0;
            int retriesDone = 0;
            bool writeFailed = false;
            int failedErrno = 0;
            static constexpr auto maxRetry = 6; // Just a random value
            for (auto retryLoop = 0; retryLoop < maxRetry; ++retryLoop)
            {
                retriesDone = retryLoop;
                writeFailed = false;
                returnedSize =
                    write(panelFileDescriptor, buffer.data(), buffer.size());
                if (returnedSize !=
                    static_cast<int>(buffer.size())) // write failure
                {
                    writeFailed = true;
                    failedErrno = errno;
                    sleep(1);
                    const std::string imValue = utils::getSystemIM();
                    if (false == utils::getLcdPanelPresentProperty(imValue))
                    {
                        return;
                    }
                    else
                    {
                        continue;
                    }
                }
                break;
            }
            if (writeFailed == true)
            {
                std::cerr << "\n I2C Write failure. Errno : " << failedErrno
                          << ". Errno description : " << strerror(failedErrno)
                          << ". Bytes written = " << returnedSize
                          << ". Actual Bytes = " << buffer.size()
                          << ". Retry = " << retriesDone << std::endl;
                std::map<std::string, std::string> additionData{};
                additionData.emplace("DESCRIPTION", strerror(failedErrno));
                additionData.emplace("CALLOUT_IIC_BUS", devPath);
                additionData.emplace("CALLOUT_IIC_ADDR", i2cAddress.str());
                additionData.emplace("CALLOUT_ERRNO",
                                     std::to_string(failedErrno));
                panel::utils::createPEL(
                    "xyz.openbmc_project.Common.Device.Error.WriteFailure",
                    "xyz.openbmc_project.Logging.Entry.Level.Warning",
                    additionData);
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

void Transport::checkAndFixBootLoaderBug()
{
    auto retries = 3;
    types::Binary readBuff;
    readBuff.resize(2);
    auto writeBuff = encoder::MessageEncoder().jumpToMainProgram();

    while (retries--)
    {
        // Read the version (2 bytes)
        // Check if we are in the bootloader: 0x42 0x4c ('B', 'L')
        auto readSize =
            ::read(panelFileDescriptor, readBuff.data(), readBuff.size());
        if (readSize != (int)readBuff.size())
        {
            std::cerr << "Failed to read panel version. Read bytes: "
                      << readSize << ", retry: " << retries
                      << ", errno: " << errno << std::endl;
            continue;
        }

        // TODO: Uncomment when we have dynamic logging
        // std::cout << "Version read from panel: " << readBuff[0] <<
        // readBuff[1]
        //          << std::endl;

        if (readBuff[0] == 'M' && readBuff[1] == 'P')
        {
            std::cout << "Validated that the panel is running the main program"
                      << std::endl;
            return;
        }

        // If we are in BL, call write to jump to MP.
        if (readBuff[0] == 'B' && readBuff[1] == 'L')
        {
            using namespace std::chrono_literals;
            std::cerr << "Panel is stuck in bootloader, attempting recovery..."
                      << std::endl;
            auto writeSize = ::write(panelFileDescriptor, writeBuff.data(),
                                     writeBuff.size());
            if (writeSize != (int)writeBuff.size())
            {
                std::cerr << "Failed to write panel jump command. Wrote bytes: "
                          << writeSize << ", retry: " << retries
                          << ", errno: " << errno << std::endl;
                std::cerr << "This is expected if the errno is 5" << std::endl;
            }
            std::this_thread::sleep_for(1s);
        }
    }

    std::cerr << "Failed to determine OR fix bootloader bug ... " << std::endl;
}

void Transport::setTransportKey(bool keyValue)
{
    transportKey = keyValue;

    if (transportKey)
    {
        // When setting key to true, check if the panel is stuck in the
        // bootloader
        checkAndFixBootLoaderBug();
    }

    if (transportKey && (panelType == types::PanelType::LCD))
    {
        doSoftReset();
        doButtonConfig();
    }

    std::cout << "\nTransport key is set to " << transportKey
              << " for the panel at " << devPath << ", " << devAddress
              << std::endl;
}

} // namespace panel
