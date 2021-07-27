#pragma once

#include "types.hpp"

#include <unistd.h>

namespace panel
{
/** @class Transport
 * @brief The Transport class is to communicate with the i2c devices.
 * When the BMC needs to communicate with the panel microcontroller, raw i2c
 * writes are made using transport class api.
 */
class Transport
{
  public:
    Transport& operator=(const Transport&) = delete;
    Transport() = delete;

    /**
     * A Constructor
     * Initialise the transport class object with the right panel device path
     * and device address based on the system type.
     */
    Transport(const std::string& devPath, const uint8_t& devAddr,
              const types::PanelType& type) :
        devPath(devPath),
        devAddress(devAddr), panelType(type)
    {
        panelI2CSetup();
    }

    /**
     * A Destructor
     * Closes the valid file descriptor when the object goes out of scope.
     */
    ~Transport()
    {
        if (panelFileDescriptor != -1)
        {
            close(panelFileDescriptor);
        }
    }

    /** @brief Write to the panel micro controller via I2C bus.
     * This api does raw i2c writes of the panel commands to the panel's micro
     * controller.
     * @param[in] buffer - data that needs to be sent to the panel.
     */
    void panelI2CWrite(const types::Binary& buffer) const;

    /** @brief Method to set the transport key
     * The transportKey boolean tells if the panel i2c bus is ready to use or
     * not. This method is to flip the key value to true/false based on the end
     * user requirement.
     * @param[in] keyValue - boolean to tell whether to activate the key or not.
     */
    void setTransportKey(bool keyValue);

    /** @brief API to setup panel's button operational characteristics. */
    void doButtonConfig();

    /** @brief Method to check and return the current status of transport key.
     * @return transportKey
     */
    inline bool isTransportKeyEnabled() const
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

    /** @brief Panel type (base/lcd) */
    const types::PanelType panelType;

    /** @brief Establish panel i2c connection
     * This api establishes the i2c bus connection to the panel micro
     * controller.
     */
    void panelI2CSetup();
};
} // namespace panel