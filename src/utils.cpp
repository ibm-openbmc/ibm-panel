#include "utils.hpp"

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
} // namespace utils
} // namespace panel