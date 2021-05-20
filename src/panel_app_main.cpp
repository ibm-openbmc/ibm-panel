#include <iostream>
#include <panel_app.hpp>
#include <sdbusplus/asio/connection.hpp>
#include <sdbusplus/asio/object_server.hpp>

void progressCodeCallBack(sdbusplus::message::message& msg)
{
    using PostCode = std::tuple<uint64_t, std::vector<uint8_t>>;

    std::string interface{};
    std::string propVal{};
    std::map<std::string, std::variant<PostCode>> propertyMap;

    msg.read(interface, propertyMap);

    // property we are looking for.
    const auto it = propertyMap.find("Value");
    if (it != propertyMap.end())
    {
        if (auto postCodeData = std::get_if<PostCode>(&(it->second)))
        {
            uint64_t src = std::get<0>(*postCodeData);
            std::ostringstream os;
            os << src;
            propVal = os.str();

            std::cout << "Progress code SRC value = " << propVal << std::endl;
        }
        else
        {
            std::cerr << "Progress code Data error" << std::endl;
        }
        // TODO:
        // send the data to display?
    }
}

int main(int, char**)
{
    boost::asio::io_context io;
    auto conn = std::make_shared<sdbusplus::asio::connection>(io);

    conn->request_name("com.ibm.PanelApp");

    auto server = sdbusplus::asio::object_server(conn);

    std::shared_ptr<sdbusplus::asio::dbus_interface> iface =
        server.add_interface("/com/ibm/panel_app", "com.ibm.panel");

    iface->register_method("Display", display);

    // register to progress code property change.
    auto progressCodeSignal = std::make_unique<sdbusplus::bus::match::match>(
        *conn,
        sdbusplus::bus::match::rules::propertiesChanged(
            "/xyz/openbmc_project/state/boot/raw0",
            "xyz.openbmc_project.State.Boot.Raw"),
        progressCodeCallBack);

    io.run();

    // ends here

    // Create the Transport class
    // Create the ButtonHandler class
    // Create the D-Bus invoker class
    // Create D-Bus signal handler
}
