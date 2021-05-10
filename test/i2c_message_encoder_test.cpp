#include "i2c_message_encoder.hpp"

#include "gtest/gtest.h"

using namespace std;
using namespace panel;
using namespace panel::encode;

TEST(MessageEncoder, checkSum)
{
    // Good case - right checksum value for the given data
    Binary dataBytes = {0xFF, 0x50, 0x00, 0x14, 0x0A};

    MessageEncoder msgEncode;
    msgEncode.calculateCheckSum(dataBytes);
    EXPECT_EQ(146, dataBytes.back());

    // when we have roll over two times
    dataBytes.clear();
    dataBytes.emplace_back(0xFF);
    dataBytes.emplace_back(0x80);
    dataBytes.emplace_back(0x00);
    dataBytes.emplace_back(0x01);
    dataBytes.emplace_back(0xFF);
    msgEncode.calculateCheckSum(dataBytes);
    EXPECT_EQ(127, dataBytes.back());

    dataBytes.clear();
    dataBytes.emplace_back(0xFF);
    dataBytes.emplace_back(0x80);
    dataBytes.emplace_back(0x00);
    dataBytes.emplace_back(0xFF);
    dataBytes.emplace_back(0xFF);
    msgEncode.calculateCheckSum(dataBytes);
    EXPECT_EQ(128, dataBytes.back());
}

TEST(MessageEncoder, displayDataWrite)
{
    MessageEncoder msgEncode;
    // Good case - The data should be encoded in the right format.(command
    // code+dataline1+dataline2+checksum)
    string line1 = "abcdefg";        // 80 bytes
    string line2 = "1234567890abcd"; // 80 bytes
    Binary validData = {0xFF, 0x80};
    line1.append(80 - line1.length(), ' ');
    line2.append(80 - line2.length(), ' ');
    copy(line1.begin(), line1.end(), back_inserter(validData));
    copy(line2.begin(), line2.end(), back_inserter(validData));
    validData.emplace_back(0xB5);
    EXPECT_EQ(validData, msgEncode.displayDataWrite(line1, line2));

    // Good case - Total 163 bytes of data should be sent for Display Data Write
    // command.
    Binary encodedData = msgEncode.displayDataWrite(line1, line2);
    size_t dataSize = encodedData.size();
    EXPECT_EQ(
        dataSize,
        163); // 163 bytes of data should present in the encoded data packet.

    // Bad case - Invalid data - bytes between 18 to 81 && 98 to 162 are not
    // filled with blank spaces
    Binary invalidData = {0xFF, 0x80};
    copy(line1.begin(), line1.end(), back_inserter(invalidData));
    copy(line2.begin(), line2.end(), back_inserter(invalidData));
    invalidData.emplace_back(0x8C);
    EXPECT_NE(invalidData, msgEncode.displayDataWrite(line1, line2));

    // Good case - when the input string has more than 80 bytes each, the extra
    // bytes are removed.
    string lineA = "hellohellohellohellohellohellohellohellohellohellohellohell"
                   "ohellohellohello123456789";
    string lineB = "world";
    validData.clear();
    validData.emplace_back(0xFF);
    validData.emplace_back(0x80);
    string lineATruncated = "hellohellohellohellohellohellohellohellohellohello"
                            "hellohellohellohellohello12345";
    copy(lineATruncated.begin(), lineATruncated.end(),
         back_inserter(validData));
    copy(lineB.begin(), lineB.end(), back_inserter(validData));
    validData.emplace_back(0xA1);
    EXPECT_EQ(validData, msgEncode.displayDataWrite(lineA, lineB));
}

TEST(MessageEncoder, buttonControl)
{
    MessageEncoder msgEncode;
    // Good case - valid data
    Binary validData = {0xFF, 0xB0, 0x02, 20, 0x01, 57};
    EXPECT_EQ(validData, msgEncode.buttonControl(0x02, 0x01));
}

int main(int argc, char** argv)
{
    ::testing::InitGoogleTest(&argc, argv);

    return RUN_ALL_TESTS();
}
