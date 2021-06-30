#include "executor.hpp"

#include "const.hpp"
#include "utils.hpp"

namespace panel
{

void Executor::executeFunction(const types::FunctionNumber funcNumber,
                               const types::FunctionalityList& subFuncNumber)
{
    // to remove warning. will be removed with subsequent commits.
    (void)subFuncNumber;

    // test output, to be removed
    switch (funcNumber)
    {
        case 20:
            execute20();

        default:
            break;
    }
}

void Executor::execute20()
{
    auto res = utils::readBusProperty<std::variant<std::string>>(
        "xyz.openbmc_project.Inventory.Manager",
        "/xyz/openbmc_project/inventory/system",
        "xyz.openbmc_project.Inventory.Decorator.Asset", "SerialNumber");

    std::string line1(16, ' ');
    std::string line2(16, ' ');

    const auto serialNumber = std::get_if<std::string>(&res);
    if (serialNumber != nullptr)
    {
        line2.replace(0, (*serialNumber).length(), *serialNumber);
    }

    // reading machine model type
    auto resValue = utils::readBusProperty<std::variant<types::Binary>>(
        "xyz.openbmc_project.Inventory.Manager",
        "/xyz/openbmc_project/inventory/system/chassis/motherboard",
        "com.ibm.ipzvpd.VSYS", "TM");

    const auto propData = std::get_if<types::Binary>(&resValue);

    if (propData != nullptr)
    {
        line1.replace(0, constants::tmKwdDataLength,
                      reinterpret_cast<const char*>(propData->data()));
    }

    // reading CCIN
    res = utils::readBusProperty<std::variant<std::string>>(
        "xyz.openbmc_project.Inventory.Manager",
        "/xyz/openbmc_project/inventory/system/chassis/motherboard",
        "xyz.openbmc_project.Inventory.Decorator.Asset", "Model");

    const auto model = std::get_if<std::string>(&res);
    if (model != nullptr)
    {
        line1.replace(11, constants::ccinDataLength, *model);
    }

    utils::sendCurrDisplayToPanel(line1, line2, transportObj);
}

} // namespace panel