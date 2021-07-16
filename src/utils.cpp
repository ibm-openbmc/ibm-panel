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
} // namespace utils
} // namespace panel