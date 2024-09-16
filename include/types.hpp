#pragma once

#include <iostream>
#include <map>
#include <sdbusplus/server.hpp>
#include <tuple>
#include <unordered_map>
#include <variant>
#include <vector>

namespace panel
{
namespace types
{

using FunctionNumber = uint8_t;
using FunctionMask = uint8_t;
using index = uint8_t;
using FunctionalityList = std::vector<FunctionNumber>;
using Byte = uint8_t;
using Binary = std::vector<Byte>;
using PanelDataTuple = std::tuple<std::string, uint8_t, std::string>;
using PanelDataMap = std::unordered_map<std::string, PanelDataTuple>;
using ItemInterfaceMap = std::map<std::string, std::variant<bool, std::string>>;
using PldmPacket = std::vector<uint8_t>;
using PdrList = std::vector<PldmPacket>;
using PICFRUPathMap = std::unordered_map<std::string, std::string>;
using PelPathAndSRCList = std::vector<std::pair<std::string, std::string>>;

// map{property::value}
using PropertyValueMap = std::map<
    std::string,
    std::variant<
        std::string, bool, uint8_t, int16_t, uint16_t, int32_t, uint32_t,
        int64_t, uint64_t, double, std::vector<std::string>,
        std::vector<std::tuple<std::string, std::string, std::string>>>>;

/* DbusInterfaceMap reference
map{InterfaceName, map{propertyName, value}}
*/
using DbusInterfaceMap = std::map<std::string, PropertyValueMap>;

using BiosProperty = std::tuple<
    std::string, bool, std::string, std::string, std::string,
    std::variant<int64_t, std::string>, std::variant<int64_t, std::string>,
    std::vector<std::tuple<std::string, std::variant<int64_t, std::string>,
                           std::string>>>;
/*baseBIOSTable reference
map{attributeName,struct{attributeType,readonlyStatus,displayname,
              description,menuPath,current,default,
              array{struct{optionstring,optionvalue}}}}
*/
using BiosBaseTableItem = std::pair<std::string, BiosProperty>;
using BiosBaseTable = std::vector<BiosBaseTableItem>;

using BiosBaseTableType =
    std::map<std::string,
             std::variant<std::monostate, std::map<std::string, BiosProperty>>>;

/* SystemParameterValues reference
 std::tuple<os_ipl_type, system_operating_mode, hmc_managed, fw_boot_side,
 hyp_switch>
 */
using SystemParameterValues =
    std::tuple<std::string, std::string, std::string, std::string, std::string>;

// map{Interface : map{property:value}}
using InterfacePropertyPair = std::pair<std::string, PropertyValueMap>;

// map{Object path, InterfacePropertyPair}
using singleObjectEntry = std::pair<sdbusplus::message::object_path,
                                    std::vector<InterfacePropertyPair>>;

using ReturnStatus = std::tuple<bool, std::string, std::string>;

/** Get managed objects for Network manager:
 * array{pair(network-object-paths :
 * array{pair(all-interfaces-of-that-obj-path :
 * map(property-of-that-interface-if-any : property-value))})}
 */
using GetManagedObjects = std::vector<singleObjectEntry>;

using AttributeValueType = std::variant<int64_t, std::string>;
using PendingAttributesItemType =
    std::pair<std::string, std::tuple<std::string, AttributeValueType>>;
using PendingAttributesType = std::vector<PendingAttributesItemType>;

enum ButtonEvent
{
    INCREMENT,
    DECREMENT,
    EXECUTE
};

enum PanelType
{
    BASE,
    LCD
};

enum class ScrollType : Byte
{
    BOTH_LEFT = 0x01,
    LINE1_LEFT = 0x11,
    LINE2_LEFT = 0x21,
};

/** @brief Structure to store and process panel version */
struct PanelVersion
{
    uint8_t major;
    uint8_t minor;

    // Delete move assignment operator and move constructor
    PanelVersion& operator=(const PanelVersion&&) = delete;
    PanelVersion(PanelVersion&&) = delete;

    /** @brief Parameterized constructor
     *
     * @param[in] mj - Major version
     * @param[in] mn - Minor version
     */
    PanelVersion(int mj, int mn)
    {
        major = mj;
        minor = mn;
    }

    /** @brief Default constructor */
    PanelVersion()
    {
        major = 0;
        minor = 0;
    }

    /** @brief Copy constructor
     *
     * @param[in] v - Input PanelVersion object
     *
     * @return Output PanelVersion object
     */
    PanelVersion(const PanelVersion& v)
    {
        major = v.major;
        minor = v.minor;
    }

    /** @brief Overload less than operator.
     *
     * @param[in] right - Right side value
     *
     * @return true if left value is less than right, false otherwise.
     */
    bool operator<(const PanelVersion& right) const
    {
        if ((major < right.major) ||
            ((major == right.major) && (minor < right.minor)))
        {
            return true;
        }
        return false;
    }

    /** @brief Overload greater than operator.
     *
     * @param[in] right - Right side value
     *
     * @return true if left value is greater than right, false otherwise.
     */
    bool operator>(const PanelVersion& right) const
    {
        if ((major > right.major) ||
            ((major == right.major) && (minor > right.minor)))
        {
            return true;
        }
        return false;
    }

    /** @brief Overload equal to operator.
     *
     * @param[in] right - Right side value
     *
     * @return true if left and right values are equal, false otherwise.
     */
    bool operator==(const PanelVersion& right) const
    {
        if ((major == right.major) && (minor == right.minor))
        {
            return true;
        }
        return false;
    }

    /** @brief Overload assignment operator
     *
     * @param[in] right - Value at the right of the assignment operator
     *
     * @return Output value
     */
    PanelVersion& operator=(PanelVersion& right)
    {
        major = right.major;
        minor = right.minor;
        return *this;
    }

    /** @brief Get PanelVersion in string.
     *
     * @return PanelVersion value as string.
     */
    std::string str() const
    {
        return std::string(1, major) + "." + std::string(1, minor);
    }
};

} // namespace types
} // namespace panel