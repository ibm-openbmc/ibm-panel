#include "i2c_message_encoder.hpp"

#include <algorithm>
#include <iostream>

using namespace std;

namespace panel
{
namespace encode
{
void MessageEncoder::calculateCheckSum(Binary& dataBuffer)
{
    Byte l_checkSum = 0;
    uint16_t l_sum = 0;
    for (Byte i : dataBuffer)
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

Binary MessageEncoder::displayDataWrite(const string& line1,
                                        const string& line2)
{
    Binary dataVector = {0xFF, 0x50};
    dataVector.reserve(163);
    copy(line1.begin(), line1.end(), back_inserter(dataVector));
    int count = 80 - line1.length();
    if (count > 0)
    {
        dataVector.insert(dataVector.end(), count, ' ');
    }
    else if (count < 0)
    {
        dataVector.erase(dataVector.end() - abs(count), dataVector.end());
    }

    copy(line2.begin(), line2.end(), back_inserter(dataVector));
    count = 80 - line2.length();
    if (count > 0)
    {
        dataVector.insert(dataVector.end(), count, ' ');
    }
    else if (count < 0)
    {
        dataVector.erase(dataVector.end() - abs(count), dataVector.end());
    }

    calculateCheckSum(dataVector);
    return dataVector;
}

Binary MessageEncoder::buttonControl(Byte buttonID, Byte buttonOperation)
{
    Binary encodedData = {0xFF, 0xB0};
    encodedData.reserve(6);
    encodedData.emplace_back(buttonID);
    encodedData.emplace_back(20); // emplace button debounce value
    encodedData.emplace_back(buttonOperation);
    calculateCheckSum(encodedData);
    return encodedData;
}

Binary MessageEncoder::scroll(Byte scrollControl)
{
    Binary encodedData = {0xFF, 0x88};
    encodedData.reserve(6);
    encodedData.emplace_back(scrollControl);
    encodedData.emplace_back(10); // emplace scroll rate
    encodedData.emplace_back(1);  // emplace scroll character count
    calculateCheckSum(encodedData);
    return encodedData;
}

Binary MessageEncoder::lampTest()
{
    Binary encodedData = {0xFF, 0x54};
    encodedData.reserve(4);
    encodedData.emplace_back(240); // emplace lamp test duration
    calculateCheckSum(encodedData);
    return encodedData;
}

Binary MessageEncoder::softReset()
{
    Binary encodedData = {0xFF, 0x00};
    encodedData.reserve(3);
    calculateCheckSum(encodedData);
    return encodedData;
}
} // namespace encode
} // namespace panel