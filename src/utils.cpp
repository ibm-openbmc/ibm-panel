#include "utils.hpp"

#include "i2c_message_encoder.hpp"

namespace panel
{
namespace utils
{
std::string binaryToHexString(const panel::types::Binary& val)
{
    std::ostringstream oss;
    for (auto i : val)
    {
        oss << std::setw(2) << std::setfill('0') << std::hex
            << static_cast<int>(i);
    }
    return oss.str();
}

void sendCurrDisplayToPanel(const std::string& line1, const std::string& line2,
                            std::shared_ptr<panel::Transport> transportObj)
{
    // couts are for debugging purpose. can be removed once the testing is done.
    std::cout << "L1 : " << line1 << std::endl;
    std::cout << "L2 : " << line2 << std::endl;

    panel::encoder::MessageEncoder encodeObj;

    auto displayPacket = encodeObj.rawDisplay(line1, line2);

    transportObj->panelI2CWrite(displayPacket);
    // TODO: After sending display packet, pause for few seconds and send scroll
    // command to scroll both the lines towards right, if the lines exceeds
    // 16characters.
}

types::SystemParameterValues readSystemParameters()
{
    auto retVal = readBusProperty<std::variant<types::BiosBaseTable>>(
        "xyz.openbmc_project.BIOSConfigManager",
        "/xyz/openbmc_project/bios_config/manager",
        "xyz.openbmc_project.BIOSConfig.Manager", "BaseBIOSTable");

    const auto baseBiosTable = std::get_if<types::BiosBaseTable>(&retVal);

    // system parameters to be read from BIOS table
    std::string OSBootType{};
    std::string systemOperatingMode{};
    std::string HMCManaged{};
    std::string FWIPLType{};
    std::string hypType{};

    if (baseBiosTable != nullptr)
    {
        for (const types::BiosBaseTableItem& item : *baseBiosTable)
        {
            const auto attributeName = std::get<0>(item);
            const auto attrValue = std::get<5>(std::get<1>(item));
            const auto val = std::get_if<std::string>(&attrValue);

            if (val != nullptr)
            {
                // TODO: How to get the information from PLDM if IPL type is
                // enabled to be displayed. Based on that execution of
                // function 01 needs to be updated to display this data.
                // Currently it is disabled in the code explicitly.
                if (attributeName == "pvm_os_boot_type")
                {
                    OSBootType = *val;
                }
                // TODO: Now this will be combination of (Power restore policy,
                // AutomaticRetryConfig and StopBootOnFault).
                else if (attributeName == "pvm_system_operating_mode")
                {
                    systemOperatingMode = *val;
                }
                else if (attributeName == "pvm_hmc_managed")
                {
                    HMCManaged = *val;
                }
                // TODO: needs to be updated. Now phyp does not use P/T.
                // BIOS table will not be used for this. Will be stored at the
                // panel end.
                // Have to come up woth new terminology. BMC does not
                // have P/T boot side.
                // To Check: When user updates the value to
                // backup image using function 02 if the display of function 01
                // does not changes accordingly then we do not need to show this
                // at all in function 01.
                else if (attributeName == "pvm_fw_boot_side")
                {
                    FWIPLType = *val;
                }
                else if (attributeName == "hb_hyp_switch")
                {
                    hypType = *val;
                }
            }
        }
    }
    else
    {
        std::cerr << "Failed to read BIOS base table" << std::endl;
    }

    return std::make_tuple(OSBootType, systemOperatingMode, HMCManaged,
                           FWIPLType, hypType);
}

} // namespace utils
} // namespace panel