#include "transport.hpp"

#include "i2c_message_encoder.hpp"

#include <fcntl.h>
#include <linux/i2c-dev.h>
#include <sys/ioctl.h>

#include <chrono>
#include <cstring>
#include <thread>

namespace panel
{
void Transport::panelI2CSetup()
{
    if ((panelFileDescriptor = open(devPath.data(), O_WRONLY | O_NONBLOCK)) ==
        -1) // open failure
    {
        auto err = errno;
        std::string error = "Failed to open device file. Errno : ";
        error += std::to_string(err);
        error += "Errno description : ";
        error += strerror(err);
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
        throw std::runtime_error(error);
    }
    std::cout << "\nSuccess opening and accessing the device path: " << devPath
              << std::endl;
}

void Transport::panelI2CWrite(const types::Binary& buffer) const
{
    if (transportKey)
    {
        if (buffer.size()) // check if the given buffer has data in it.
        {
            auto returnedSize =
                write(panelFileDescriptor, buffer.data(), buffer.size());
            if (returnedSize !=
                static_cast<int>(buffer.size())) // write failure
            {
                std::cerr << "\n I2C Write failure. Errno : " << errno
                          << ". Errno description : " << strerror(errno)
                          << ". Bytes written = " << returnedSize
                          << ". Actual Bytes = " << buffer.size() << std::endl;
            }
        }
        else
        {
            std::cerr << "\n Buffer empty. Skipping I2C Write." << std::endl;
        }
    }
    else
    {
        std::cout << "\n Transport Key is inactive. Cannot do raw i2c writes."
                  << std::endl;
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
    std::this_thread::sleep_for(100ms);
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
    std::cout << "\nTransport key is set to " << transportKey << std::endl;
}

} // namespace panel
