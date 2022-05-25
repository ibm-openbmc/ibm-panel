#pragma once

#include "panel_state_manager.hpp"
#include "transport.hpp"

#include <linux/input.h>

#include <sdbusplus/asio/connection.hpp>
#include <string>
#include <vector>

namespace panel
{

/**
 * @brief A class to listen for input subsystem, read the event and pass the
 * appropriate button event to state handler.
 */
class ButtonHandler
{
  public:
    /* Deleted Api's*/
    ButtonHandler(const ButtonHandler&) = delete;
    ButtonHandler& operator=(const ButtonHandler&) = delete;
    ButtonHandler(ButtonHandler&&) = delete;

    /*Destructor*/
    ~ButtonHandler()
    {
        if (fd > 0)
        {
            close(fd);
        }
    }

    /**
     * @brief Constructor.
     * @param[in] path - Path of the device.
     * @param[in] io - Boost asio io_context object pointer.
     * @param[in] transport - Transport Object to call transport functions.
     * @param[in] stateManager - State manager object to call
     * @param[in] i2cBusPath - i2c bus path to communicate with panel.
     * increment/decrement/execute methods.
     */
    ButtonHandler(
        const std::string& path, std::shared_ptr<boost::asio::io_context>& io,
        std::shared_ptr<Transport> transport,
        std::shared_ptr<state::manager::PanelStateManager> stateManager,
        const std::string& i2cBusPath);

  private:
    /** @brief Api to perform async read operation on the device path.
     */
    void performAsyncRead();

    /** @brief Callback api to process the input event received.
     *  @param[in] ec - error code to capture error in call.
     *  @param[in] bytesTransferred - number of bytes transferred.
     */
    void processInputEvent(const boost::system::error_code& ec,
                           size_t bytesTransferred);

    /* Device path*/
    std::string devicePath;

    /* boost asio io_context object pointer*/
    std::shared_ptr<boost::asio::io_context> io;

    /* Stream descriptor for asynchronous operation on file descriptor*/
    std::unique_ptr<boost::asio::posix::stream_descriptor> streamDescriptor =
        nullptr;

    /* buffer of input event required for async_read operation*/
    std::vector<input_event> ipEvent;

    /* transport object required for calling transport functions */
    std::shared_ptr<Transport> transport;

    /* state manager object to call increment/decrement/execute functions */
    std::shared_ptr<state::manager::PanelStateManager> stateManager;

   /* The /dev/ path to the I2C bus that is used to communicate with the panel.
    * This will be used for callout purposes. */
   std::string busPath;

    /* file descriptor */
    int fd = -1;

}; // class ButtonHandler
} // namespace panel