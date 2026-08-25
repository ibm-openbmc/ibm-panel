#pragma once

#include "types.hpp"

#include <unistd.h>

namespace panel
{
/** @class Transport
 *
 * @brief The Transport class is to communicate with the i2c devices.
 * When the BMC needs to communicate with the panel microcontroller, raw i2c
 * writes are made using transport class api.
 */
class Transport
{
  public:
    /**
     * Deleted methods
     */
    Transport(const Transport&) = delete;
    Transport& operator=(const Transport&) = delete;
    Transport& operator=(Transport&&) = delete;
    Transport(Transport&&) = delete;

    /**
     * @brief Default constructor
     *
     * Creates a Transport instance with no device path or address.
     * The transport key is left false; intended for testing / stub use.
     */
    Transport() noexcept;

    /**
     * @brief Parameterised constructor
     *
     * Initialises the Transport object with the panel's device path, I2C
     * address and FRU inventory path.
     *
     * @param[in] devPath     - Sysfs path to the I2C device node.
     * @param[in] devAddr     - 7-bit I2C slave address of the panel.
     * @param[in] objectPath  - D-Bus FRU inventory object path.
     *
     * @throw bad_alloc, runtime_error
     */
    Transport(const std::string& devPath, const uint8_t& devAddr,
              const std::string& objectPath);

    /**
     * @brief Destructor
     *
     * Closes the file descriptor held for the I2C device node, if open.
     */
    ~Transport() noexcept;

    /**
     * @brief Write raw bytes to the panel micro-controller via I2C
     *
     * Sends the given buffer over the I2C bus to the panel.
     *
     * If any failure occured while writing, a PEL is logged.
     *
     * @param[in] buffer - Byte sequence to send to the panel.
     */
    void panelI2CWrite(const types::Binary& buffer) const noexcept;

    /**
     * @brief Set the transport key to enable or disable I2C communication
     *
     * When set to true the I2C bus is considered ready for use; false disables
     * all writes.
     *
     * @param[in] keyValue - true to enable the transport, false to disable.
     */
    void setTransportKey(bool keyValue) noexcept;

    /**
     * @brief Return the current state of the transport key
     *
     * @return true if the transport key is enabled, false otherwise.
     */
    inline bool isTransportKeyEnabled() const noexcept
    {
        return transportKey;
    }

  private:
    /** @brief The panel's file descriptor */
    int panelFileDescriptor = -1;

    /** @brief Panel device path */
    const std::string devPath;

    /** @brief Panel device address */
    const uint8_t devAddress;

    /** @brief Key to check the availability of transport class.
     * The transportKey tells if the panel i2c bus is ready to use or not.
     * If key value is true, the transport class can be used to access the panel
     * i2c bus. False otherwise.
     */
    bool transportKey = false;

    /** @brief i2cAddress and devAddress are same in value but differs in type.
     * This is required to log CALLOUT_IIC_ADDR in PEL.*/
    std::string i2cAddress{};

    /** @brief panel FRU Dbus object path */
    const std::string fruPath;

    /**
     * @brief Establish the I2C connection to the panel micro-controller
     *
     * Validates that devPath and devAddress are set, then:
     *  - Formats devAddress as a hex string and stores it in i2cAddress for
     *    use in PEL callout data.
     *  - Opens the device node at devPath with O_RDWR | O_NONBLOCK and stores
     *    the resulting file descriptor in panelFileDescriptor. On failure a PEL
     *    (com.ibm.Panel.Error.I2CSetupFailure / Warning) is logged.
     *  - Calls ioctl(I2C_SLAVE) to bind the file descriptor to devAddress. On
     *    failure a PEL (com.ibm.Panel.Error.I2CSetupFailure / Warning) is
     *    logged.
     *
     * @throws std::runtime_error
     */
    void panelI2CSetup();
};
} // namespace panel
