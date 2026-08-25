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
};
} // namespace panel