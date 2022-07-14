#pragma once

#include "types.hpp"

#include <string>

namespace panel
{
namespace encoder
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

    /** @brief Method to encode display command.
     * This method encodes the "Display Data Write" command data which is sent
     * to the micro controller by the BMC. The command code of
     * Display Data Write is "0xFF80". The encoded data packet contains the
     * command code(0xFF80) + line1 and line2 strings and the calculated
     * checksum. The packet as a whole is returned to the caller.
     * @param[in] line1 - Line 1 can contain a max of 80 ascii characters , of
     * which 16 ascii characters are displayed at a time on the LCD panel's
     * upper line.
     * @param[in] line2 - Line 2 can contain a max of 80 ascii characters , of
     * which 16 ascii characters are displayed at a time on the LCD panel's
     * second line.
     * @return Encoded data packet of "Display" command.
     */
    Binary rawDisplay(const std::string& line1, const std::string& line2);

    /** @brief Method to calculate checksum of the panel command's data.
     * This method calculates and return the checksum of the panel command's
     * data. The checksum is appended to the end of each IIC Panel command's
     * encoded data packet to ensure data integrity.
     * @param[io] dataBuffer - vector of data for which the checksum needs be
     * calculated. The calculated checksum will be appended to the buffer.
     */
    void calculateCheckSum(Binary& dataBuffer);

    /** @brief Method which encodes button control command data.
     * The Button control command is used to setup the panel's button
     * operational characteristics. A button is not active at the panel until
     * its operational characteristics are specified. This method encodes the
     * button control command code 0xFFB0; the button debounce rate (which is
     * defaulted to 20), denotes the consecutive samples required for
     * transition; the button id and the button operation along with the
     * calculated checksum. The encoded data packet is sent to the micro
     * controller by the BMC.
     * @param[in] buttonID - the button id. (0x00 – Increment; 0x01 – Decrement;
     * 0x02 - Enter; 0x03 till 0xFF – Reserved)
     * @param[in] buttonOperation - the button operation field. (0x00 - Single
     * execution; 0x01 - Double execution; 0x02 till 0xFF – Reserved)
     * @return Encoded data packet of Button control command.
     */
    Binary buttonControl(Byte buttonID, Byte buttonOperation);

    /** @brief Internal Scroll command encode api
     * The Internal Scroll command is used to start/stop display scrolling and
     * to define scroll characteristics. The Internal Scroll command is used to
     * scroll LCD data loaded by the Display Data Write command. The encoded
     *data packet contains the command code of scroll command which is 0xFF88;
     *the given scroll control type; the scroll rate which is defaulted to
     *10msec; the scroll character count which denotes number of characters to
     *be scrolled which is defaulted to 1 character at a time; and the
     *calculated checksum. The encoded data packet is sent to the micro
     *controller by the BMC.
     * @param[in] scrollControl - scroll control type. Possible input values
     *are: 0x00 - Both right continuously at rate specified. 0x01 - Both left
     *continuously at rate specified. 0x02 - Both right the number of characters
     *specified. 0x03 - Both left the number of characters specified. 0x10 -
     *Line 1 right continuously at rate specified. 0x11 - Line 1 left
     *continuously at rate specified. 0x12 - Line 1 right the number of
     *characters specified. 0x13 - Line 1 left the number of characters
     *specified. 0x20 - Line 2 right continuously at rate specified. 0x21 - Line
     *2 left continuously at rate specified. 0x22 - Line 2 right the number of
     *characters specified. 0x23 - Line 2 left the number of characters
     *specified. Others - Stop scrolling.
     * @return Encoded data packet of internal scroll command.
     */
    Binary scroll(Byte scrollControl);

    /** @brief Lamp test command encode api
     * The Lamp Test command is used to perform a lamp test on all illumination
     * elements (LED, LCD) on the converged Panel.
     * The encoded data packet contains the command code of lamp test command
     * (0xFF54); the lamp test duration which is defaulted to 240seconds; lamp
     * test period and lamp test on period along with the calculated checksum.
     * @return Encoded data packet of lamp test command.
     */
    Binary lampTest();

    /** @brief Soft Reset command encode api
     * The Panel Code Soft Reset command is used to perform a soft reset of the
     * Panel micro-controller. This will re-initialize the Panel micro-code to
     * its start-up values. The encoded data packet contains the command code of
     * soft reset command(0xFF00) and the calculated checksum.
     * @return Encoded data packet of soft reset command.
     */
    Binary softReset();

    /** @brief Flash Update command encode api
     * The encoded data packet contains the command code of flash update(0xFF30)
     * and the calculated checksum.
     * @return Encoded data packet of flash update command.
     */
    Binary flashUpdate();

    /** @brief Jump from bootloader to main program
     * The encoded data packet contains the command code of jumping to main
     * program (0xFF25) and the calculated checksum.
     * @return Encoded data packet of jump to main program command.
     */
    Binary jumpToMainProgram();

    /** @brief Display version command encode api
     * The encoded data packet contains the command code of display version
     * command (0xFF50) and the calculated checksum.
     * @return Encoded data packet of display version command.
     */
    Binary displayVersionCmd();
};
} // namespace encoder
} // namespace panel