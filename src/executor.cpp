#include "executor.hpp"

#include "const.hpp"
#include "utils.hpp"

namespace panel
{

void Executor::executeFunction(const types::FunctionNumber funcNumber,
                               const types::FunctionalityList& subFuncNumber)
{
    // test output, to be removed
    std::cout << "Function Number" << int(funcNumber) << std::endl;
    (void)subFuncNumber; // to remove unused variable error

    switch (funcNumber)
    {
        case 1:
            execute01();
            break;

        case 20:
            execute20();
            break;

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

    utils::sendCurrDisplayToPanel(line1, line2, transport);
}

bool Executor::isOSIPLTypeEnabled() const
{
    // TODO: Check with PLDM how they will communicate if IPL type is
    // enabled or disabled. Till then return dummy value as false.
    return false;
}

void Executor::execute01()
{
    const auto sysValues = utils::readSystemParameters();

    if (!std::get<0>(sysValues).empty() && !std::get<1>(sysValues).empty() &&
        !std::get<2>(sysValues).empty() && !std::get<3>(sysValues).empty() &&
        !std::get<4>(sysValues).empty())
    {
        std::string line1(16, ' ');
        std::string line2(16, ' ');

        // function number
        line1.replace(0, 2, "01");

        if (isOSIPLTypeEnabled())
        {
            // OS IPL Type
            line1.replace(4, 1, std::get<0>(sysValues).substr(0, 1));
        }

        // Operating mode
        line1.replace(7, 1, std::get<1>(sysValues).substr(0, 1));

        // hypervisor type
        if (std::get<4>(sysValues) == "PowerVM")
        {
            line1.replace(12, 3, "PVM");
        }
        else
        {
            line1.replace(12, std::get<4>(sysValues).length(),
                          std::get<4>(sysValues));
        }

        // HMC Managed
        if (std::get<2>(sysValues) == "1")
        {
            line2.replace(0, 5, "HMC=1");
        }

        // Boot side. This needs to be renamed later.
        line2.replace(12, 1, std::get<3>(sysValues).substr(0, 1));

        utils::sendCurrDisplayToPanel(line1, line2, transport);
        return;
    }

    std::cerr << "Error reading system parameters" << std::endl;
}
} // namespace panel