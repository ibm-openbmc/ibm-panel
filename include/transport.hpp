#pragma once

#include "types.hpp"

#include <unistd.h>

#include <string>

namespace panel
{
/** @class Transport
 *
 * @brief The Transport class is to communicate with the i2c devices
 *
 * When the BMC needs to communicate with the panel microcontroller, raw i2c
 * writes are made using transport class api.
 */
class Transport
{
  public:
    /** @brief Deleted special members */
    Transport(const Transport&) = delete;
    Transport& operator=(const Transport&) = delete;
    Transport& operator=(Transport&&) = delete;
    Transport(Transport&&) = delete;

    /**
     * @brief Default constructor
     *
     * Creates a Transport instance with no device details, intended to use for
     * testing.
     */
    Transport() noexcept;

    /** @brief Destructor */
    ~Transport() noexcept;

    /**
     * @brief API to set the transport key
     *
     * The transportKey boolean tells if the i2c bus is ready to use or
     * not. This method is to flip the key value to true/false based on
     * the requirement.
     *
     * @param[in] keyValue - boolean to activate/deactivate the transportKey.
     */
    void setTransportKey(bool keyValue);

  private:
    /**
     * @brief The transportKey tells if the i2c bus is ready to use or not
     *
     * If key value is true, the transport class can be used to access the
     * i2c bus. Otherwise transport class can't be used for i2c operation.
     */
    bool transportKey = false;
};
} // namespace panel