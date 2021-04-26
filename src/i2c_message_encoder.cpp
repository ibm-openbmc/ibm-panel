#include "i2c_message_encoder.hpp"

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
} // namespace encode
} // namespace panel