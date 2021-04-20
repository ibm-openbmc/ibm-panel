#include <panel_app.hpp>
#include <sdbusplus/asio/connection.hpp>
#include <sdbusplus/asio/object_server.hpp>

int main(int , char** )
{
    boost::asio::io_context io;
    auto conn = std::make_shared<sdbusplus::asio::connection>(io);
    conn->request_name("com.ibm.PanelApp");

    auto server = sdbusplus::asio::object_server(conn);

    std::shared_ptr<sdbusplus::asio::dbus_interface> iface =
        server.add_interface("/com/ibm/panel_app",
                             "com.ibm.panel");

    iface->register_method("Display", display);

    // Create the Transport class
    // Create the ButtonHandler class
    // Create the D-Bus invoker class
    // Create D-Bus signal handler
}
