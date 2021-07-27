#include "button_handler.hpp"

#include "panel_state_manager.hpp"

#include <assert.h>

#include <boost/asio.hpp>
#include <ranges>

namespace panel
{

ButtonHandler::ButtonHandler(
    const std::string& path, std::shared_ptr<boost::asio::io_context>& io,
    std::shared_ptr<Transport> transport,
    std::shared_ptr<state::manager::PanelStateManager> stateManager) :
    devicePath(path),
    io(io), transport(transport), stateManager(stateManager)
{
    // TODO update this to actual size needed.
    ipEvent.resize(10);

    if (!devicePath.empty())
    {
        // open stream in read and non blocking mode
        fd = open(devicePath.c_str(), O_RDONLY | O_NONBLOCK);

        if (fd < 0)
        {
            throw std::runtime_error("Failed to open the input path. Button "
                                     "handler creation failed");
        }

        // register stream descriptor
        streamDescriptor =
            std::make_unique<boost::asio::posix::stream_descriptor>(*io, fd);

        // register async read callback.
        performAsyncRead();
    }
    else
    {
        assert("Empty device path");
    }
}

void ButtonHandler::performAsyncRead()
{
    boost::asio::async_read(
        *streamDescriptor, boost::asio::buffer(ipEvent),
        boost::asio::transfer_at_least(sizeof(input_event)),
        [this](const boost::system::error_code& ec, size_t bytesTransferred) {
            processInputEvent(ec, bytesTransferred);
        });
}

void ButtonHandler::processInputEvent(const boost::system::error_code& ec,
                                      size_t bytesTransferred)
{
    // re-register the call back
    performAsyncRead();

    // process only if bytes read is atleast size of input event structure.
    if (!ec)
    {
        auto const numOfEvents = bytesTransferred / sizeof(input_event);

        for (auto& ev : std::views::counted(ipEvent.begin(), numOfEvents))
        {
            std::cout << "Received event type: " << ev.type << std::endl
                      << " event code = " << ev.code << std::endl
                      << " event value = " << ev.value << std::endl;

            // process only for release event i.e. 1
            if (ev.value == 0)
            {
                return;
            }
            switch (ev.code)
            {
                case BTN_NORTH:
                    stateManager->processPanelButtonEvent(
                        types::ButtonEvent::INCREMENT);
                    break;

                case BTN_SOUTH:
                    stateManager->processPanelButtonEvent(
                        types::ButtonEvent::DECREMENT);
                    break;

                case BTN_SELECT:
                    stateManager->processPanelButtonEvent(
                        types::ButtonEvent::EXECUTE);
                    break;

                default: /* unknown button event*/
                    break;
            }
        }
    }
}

} // namespace panel