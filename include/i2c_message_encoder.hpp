#include "types.hpp"

#include <bitset>
#include <string>
#include <vector>

namespace panel
{
namespace encode
{
using namespace types;
class MessageEncoder
{
  public:
    MessageEncoder(const MessageEncoder&) = delete;
    MessageEncoder& operator=(const MessageEncoder&) = delete;
    MessageEncoder(MessageEncoder&&) = delete;
    MessageEncoder() = default;
    ~MessageEncoder() = default;

    /** @brief Method to display data on panel.
     * This method encodes the "Display Data Write" command data which is sent
     * to the slave micro controller by the master BMC. The command code of
     * Display Data Write is "0xFF80".
     *
     * @param[io] line1 - the first 15 bytes of data that needs to be displayed
     * on first line of the panel.
     * @param[io] line2 - the first 15 bytes of data that needs to be displayed
     * on second line of the panel.
     * @return Encoded data packet of "Display Data Write" command.
     */
    Binary displayDataWrite(const std::string& line1, const std::string& line2);

    /** @brief Method to calculate checksum of the panel command's data.
     * This method calculates and returns the checksum of the panel command's
     * data. The checksum is appended to the end of each IIC Panel command to
     * ensure data integrity.
     * @param[io] dataBuffer - vector of data for which the checksum needs be
     * calculated. The calculated checksum will be appended to the buffer.
     */
    void calculateCheckSum(Binary& dataBuffer);

    /** @brief Method which encodes button control command data.
     * The Button control command is used to setup the panel's button
     * operational characteristics. A button is not active at the panel until
     * its operational characteristics are specified. This method encodes the
     * data of button control command whose code is 0xFFB0. The encoded data
     * packet is sent to the slave micro controller by the master BMC.
     * @param[in] buttonID - the button id which takes values from 0x00 till
     * 0x02. Other values are reserved.
     * @param[in] buttonOperation - the button operation which takes 0x00 & 0x01
     * as its value and others are reserved.
     * @return Encoded data packet of Button control command.
     */
    Binary buttonControl(Byte buttonID, Byte buttonOperation);
};
} // namespace encode
} // namespace panel