#include "dbus_call.hpp"

#include <CLI/CLI.hpp>

#include <iostream>

int main(int argc, char** argv)
{
    std::string input{};
    CLI::App app{"Command line tool for simulating panel functions "};
    auto state = app.add_option(
        " -b", input,
        " Simulating button press"
        " Increment/Decrement/Execute with UP/DOWN/EXECUTE respectively");
    CLI11_PARSE(app, argc, argv);

    try
    {
        if (*state)
        {
            panel::tool::btnEventDbusCall(input);
        }
        else
        {
            throw std::runtime_error(
                "One of the valid options is required. Refer "
                "--help for list of options.");
        }
    }
    catch (std::exception& e)
    {
        std::cerr << e.what();
    }
    return 0;
}
