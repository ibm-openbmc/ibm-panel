#include "i2c_message_encoder.hpp"

#include <algorithm>

using namespace std;

namespace panel
{
namespace encoder
{
void MessageEncoder::calculateCheckSum(Binary& dataBuffer)
{
    Byte l_checkSum = 0;
    uint16_t l_sum = 0;
    for (auto i : dataBuffer)
    {
        l_sum += i;
        if (l_sum & 0xFF00)
        {
            l_sum &= 0x00FF;
            l_sum += 1;
        }
    }
    l_checkSum = static_cast<Byte>(l_sum & 0x00FF);
    l_checkSum = ~l_checkSum;
    l_checkSum += 1;
    dataBuffer.emplace_back(l_checkSum);
}

Binary MessageEncoder::rawDisplay(const string& line1, const string& line2)
{
    Binary encodedData;
    encodedData.reserve(163);
    encodedData.emplace_back(0xFF);
    encodedData.emplace_back(0x80);

    copy(line1.begin(), line1.end(), back_inserter(encodedData));
    int count = 80 - line1.length();
    if (count > 0)
    {
        encodedData.insert(encodedData.end(), count, ' ');
    }
    else if (count < 0)
    {
        encodedData.erase(encodedData.end() - abs(count), encodedData.end());
    }

    copy(line2.begin(), line2.end(), back_inserter(encodedData));
    count = 80 - line2.length();
    if (count > 0)
    {
        encodedData.insert(encodedData.end(), count, ' ');
    }
    else if (count < 0)
    {
        encodedData.erase(encodedData.end() - abs(count), encodedData.end());
    }

    calculateCheckSum(encodedData);
    return encodedData;
}

Binary MessageEncoder::buttonControl(Byte buttonID, Byte buttonOperation)
{
    Binary encodedData;
    encodedData.reserve(6);
    encodedData.emplace_back(0xFF);
    encodedData.emplace_back(0xB0);
    encodedData.emplace_back(buttonID);
    encodedData.emplace_back(20); // emplace button debounce value
    encodedData.emplace_back(buttonOperation);
    calculateCheckSum(encodedData);
    return encodedData;
}

Binary MessageEncoder::scroll(Byte scrollControl)
{
    Binary encodedData;
    encodedData.reserve(6);
    encodedData.emplace_back(0xFF);
    encodedData.emplace_back(0x88);
    encodedData.emplace_back(scrollControl);
    encodedData.emplace_back(10); // emplace scroll rate
    encodedData.emplace_back(1);  // emplace scroll character count
    calculateCheckSum(encodedData);
    return encodedData;
}

Binary MessageEncoder::lampTest()
{
    Binary encodedData;
    encodedData.reserve(4);
    encodedData.emplace_back(0xFF);
    encodedData.emplace_back(0x54);
    encodedData.emplace_back(240); // emplace lamp test duration
    calculateCheckSum(encodedData);
    return encodedData;
}

Binary MessageEncoder::softReset()
{
    Binary encodedData;
    encodedData.reserve(3);
    encodedData.emplace_back(0xFF);
    encodedData.emplace_back(0x00);
    calculateCheckSum(encodedData);
    return encodedData;
}

Binary MessageEncoder::flashUpdate()
{
    Binary encodedData;
    encodedData.reserve(3);
    encodedData.emplace_back(0xFF);
    encodedData.emplace_back(0x30);
    calculateCheckSum(encodedData);
    return encodedData;
}

Binary MessageEncoder::displayVersionCmd()
{
    Binary encodedData;
    encodedData.reserve(3);
    encodedData.emplace_back(0xFF);
    encodedData.emplace_back(0x50);
    calculateCheckSum(encodedData);
    return encodedData;
}

} // namespace encoder
} // namespace panel