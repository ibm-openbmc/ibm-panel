#include "transport.hpp"

#include "base_fw_latest.hpp"
#include "const.hpp"
#include "i2c_message_encoder.hpp"
#include "lcd_fw_latest.hpp"
#include "utils.hpp"

#include <fcntl.h>
#include <linux/i2c-dev.h>
#include <sys/ioctl.h>

#include <chrono>
#include <cstring>
#include <sstream>
#include <thread>

using namespace std::chrono_literals;

namespace panel
{
void Transport::panelI2CSetup()
{
    std::ostringstream byteStream;
    byteStream << "0x" << std::hex << std::uppercase
               << static_cast<int>(devAddress);
    i2cAddress = byteStream.str();

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
        additionData.emplace("CALLOUT_IIC_ADDR", i2cAddress);
        additionData.emplace("CALLOUT_ERRNO", std::to_string(err));
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
        additionData.emplace("CALLOUT_IIC_ADDR", i2cAddress);
        additionData.emplace("CALLOUT_ERRNO", std::to_string(err));
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
                returnedSize = write(panelFileDescriptor, buffer.data(),
                                     buffer.size());
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
                additionData.emplace("CALLOUT_IIC_ADDR", i2cAddress);
                additionData.emplace("CALLOUT_ERRNO",
                                     std::to_string(failedErrno));
                panel::utils::createPEL(
                    constants::deviceWriteFailure,
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
        auto readSize = ::read(panelFileDescriptor, readBuff.data(),
                               readBuff.size());
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

bool Transport::readPanelVersion(types::Binary& versionBuffer) const
{
    const size_t versionSize = 6;
    versionBuffer.resize(versionSize);

    auto readSize = ::read(panelFileDescriptor, versionBuffer.data(),
                           versionSize);

    if (readSize != versionSize)
    {
        std::cerr << "Failed to read panel current version [" << devPath << ", "
                  << i2cAddress << "]. Bytes read: " << readSize
                  << ", errno: " << errno << std::endl;
        if (constants::errnoNoDeviceOrAddress == errno)
        {
            // creating errorlog as device not found, it could be due to
            // hardware issue or it could be due to blank firmware.
            std::map<std::string, std::string> errorInfo{};
            errorInfo.emplace("CALLOUT_IIC_BUS", devPath);
            errorInfo.emplace("CALLOUT_IIC_ADDR", i2cAddress);
            errorInfo.emplace("CALLOUT_ERRNO", std::to_string(errno));
            utils::createPEL(constants::deviceReadFailure,
                             "xyz.openbmc_project.Logging.Entry.Level.Error",
                             errorInfo);
        }
        return false;
    }
    return true;
}

void Transport::doFWUpdate() const
{
    // Read current microcode version
    types::Binary versionBuffer;
    if (!readPanelVersion(versionBuffer))
    {
        return;
    }

    if (versionBuffer[0] != 'M' && versionBuffer[1] != 'P')
    {
        if (versionBuffer[0] != 'B' && versionBuffer[1] != 'L')
        {
            logCodeUpdateError(
                "FW Code requires minimum version to proceed with code "
                "update. Code update fails.",
                0, "Not reached the Main Program",
                constants::codeUpdateFailure);
            return;
        }
        std::cerr << "\n The Op-panel at " << devPath << ", " << i2cAddress
                  << " has not reached the Main Program. Aborting code update."
                  << std::endl;
        return;
    }

    types::PanelVersion currentVersion;

    if (versionBuffer[0] == 'M' && versionBuffer[1] == 'P')
    {
        types::PanelVersion v(versionBuffer[3], versionBuffer[5]);
        currentVersion = v;
    }

    std::cout << "\n The current version of Op-panel at " << devPath << ", "
              << i2cAddress << " is " << currentVersion.str() << std::endl;

    types::PanelVersion maxVersion =
        (panelType == types::PanelType::LCD)
            ? types::PanelVersion(constants::maxLCDVersion)
            : types::PanelVersion(constants::maxBaseVersion);

    if (currentVersion == maxVersion || currentVersion > maxVersion)
    {
        std::cout << "\nOp-panel at " << devPath << ", " << i2cAddress
                  << " has the latest version " << currentVersion.str()
                  << ". Code update not required." << std::endl;
        return;
    }
    else if (currentVersion < constants::minPanelVersion)
    {
        logCodeUpdateError(
            "FW Code requires minimum version to proceed with code update. "
            "Code update fails.",
            0, "In Main Program", constants::codeUpdateFailure);
        return;
    }

    if (!gotoBootloader())
    {
        std::cerr << "\nFailed to switch to boot loader. Code update failed "
                     "for Op-panel at "
                  << devPath << ", " << i2cAddress << std::endl;
        return;
    }

    if (!updateFlash())
    {
        std::cerr << "\nFlash failed. Aborting code update for Op-panel at "
                  << devPath << ", " << i2cAddress << std::endl;
        return;
    }

    if (!gotoMainProgram())
    {
        std::cerr << "\nFailed jumping to main program after a code update for "
                     "Op-panel at "
                  << devPath << ", " << i2cAddress << std::endl;
        return;
    }
    else
    {
        types::Binary mpVersion;

        if (readPanelVersion(mpVersion) && mpVersion[0] == 'M' &&
            mpVersion[1] == 'P')
        {
            types::PanelVersion v(mpVersion[3], mpVersion[5]);
            currentVersion = v;

            if (currentVersion == maxVersion)
            {
                std::cout
                    << "\nFirmware update successful to the latest version "
                    << currentVersion.str() << " for the op-panel at "
                    << devPath << ", " << i2cAddress << std::endl;
                return;
            }
            logCodeUpdateError("Failed updating firmware to the latest version",
                               0, "In Main Program",
                               constants::deviceWriteFailure);
            return;
        }
    }
}

void Transport::setTransportKey(bool keyValue)
{
    transportKey = keyValue;

    if (transportKey)
    {
        // When setting key to true, check if the panel is stuck in the
        // bootloader
        checkAndFixBootLoaderBug();
        doFWUpdate();
    }

    if (transportKey && (panelType == types::PanelType::LCD))
    {
        doSoftReset();
        doButtonConfig();
    }

    std::cout << "\nTransport key is set to " << transportKey
              << " for the panel at " << devPath << ", " << i2cAddress
              << std::endl;
}

bool Transport::gotoBootloader() const
{
    auto writeBuff = encoder::MessageEncoder().jumpToBootLoader();

    auto writeSize = ::write(panelFileDescriptor, writeBuff.data(),
                             writeBuff.size());
    if (writeSize != (int)writeBuff.size())
    {
        logCodeUpdateError(
            "Failed jumping to panel boot loader. Code update fails.", errno,
            "In Main Program", constants::deviceWriteFailure);
        return false;
    }

    std::this_thread::sleep_for(1s);

    types::Binary blVersion;

    if (!readPanelVersion(blVersion) ||
        (blVersion[0] != 'B' && blVersion[1] != 'L'))
    {
        logCodeUpdateError(
            "Failed to read Boot Loader version. Code update fails.", 0,
            "In Boot Loader", constants::deviceReadFailure);
        return false;
    }
    return true;
}

bool Transport::updateFlash() const
{
    auto maxAvailBytes = 0;
    const void* lcdPtr = &(firmware::lcd);
    const void* basePtr = &(firmware::base);

    const void* fwUpdateBuffer = (panelType == types::PanelType::LCD) ? lcdPtr
                                                                      : basePtr;
    const int fwCodeSize = (panelType == types::PanelType::LCD)
                               ? (firmware::lcd).size()
                               : (firmware::base).size();

    for (auto chunkIndex = 0; chunkIndex < fwCodeSize;
         chunkIndex += maxAvailBytes)
    {
        if (fwCodeSize - chunkIndex < constants::maxFlashWriteChunk)
        {
            maxAvailBytes = fwCodeSize - chunkIndex;
        }
        else
        {
            maxAvailBytes = constants::maxFlashWriteChunk;
        }

        auto sizeWritten =
            ::write(panelFileDescriptor,
                    (static_cast<const uint8_t*>(fwUpdateBuffer) + chunkIndex),
                    maxAvailBytes);

        if (sizeWritten != maxAvailBytes)
        {
            logCodeUpdateError(
                "Failed to write a byte chunk while flashing. Code update "
                "fails.",
                errno, "In Boot Loader", constants::deviceWriteFailure);
            return false;
        }
    }
    return true;
}

bool Transport::gotoMainProgram() const
{
    auto writeBuff = encoder::MessageEncoder().jumpToMainProgram();

    auto writeSize = ::write(panelFileDescriptor, writeBuff.data(),
                             writeBuff.size());

    std::this_thread::sleep_for(1s);

    if (writeSize != static_cast<int>(writeBuff.size()) && errno != EIO)
    {
        logCodeUpdateError(
            "Failed jumping to Main program after a code update.", errno,
            "In Boot Loader", constants::deviceWriteFailure);
        return false;
    }
    return true;
}

void Transport::logCodeUpdateError(const std::string& description,
                                   const int err, const std::string& control,
                                   const std::string& pelIntf) const
{
    std::map<std::string, std::string> codeUpdatePELData;

    codeUpdatePELData.emplace("DESCRIPTION", description);
    codeUpdatePELData.emplace("MINIMUM_VERSION",
                              constants::minPanelVersion.str());

    if (panelType == types::PanelType::LCD)
    {
        codeUpdatePELData.emplace("MAXIMUM_VERSION",
                                  constants::maxLCDVersion.str());
        codeUpdatePELData.emplace("CHIP TYPE", "LCD PIC");
    }
    else if (panelType == types::PanelType::BASE)
    {
        codeUpdatePELData.emplace("MAXIMUM_VERSION",
                                  constants::maxBaseVersion.str());
        codeUpdatePELData.emplace("CHIP TYPE", "BootFail PIC");
    }

    codeUpdatePELData.emplace("CONTROL", control);

    if (pelIntf != constants::codeUpdateFailure)
    {
        codeUpdatePELData.emplace("CALLOUT_IIC_BUS", devPath);
        codeUpdatePELData.emplace("CALLOUT_IIC_ADDR", i2cAddress);
        codeUpdatePELData.emplace("CALLOUT_ERRNO", std::to_string(err));
    }
    else
    {
        codeUpdatePELData.emplace("CALLOUT_INVENTORY_PATH", fruPath);
    }

    utils::createPEL(pelIntf, "xyz.openbmc_project.Logging.Entry.Level.Error",
                     codeUpdatePELData);
}
} // namespace panel
