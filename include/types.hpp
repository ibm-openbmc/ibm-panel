#pragma once

#include <iostream>
#include <map>
#include <tuple>
#include <unordered_map>
#include <variant>
#include <vector>

namespace panel
{
namespace types
{

using FunctionNumber = uint8_t;
using FunctionalityList = std::vector<FunctionNumber>;
using Byte = uint8_t;
using Binary = std::vector<Byte>;
using PanelDataTuple = std::tuple<std::string, uint8_t, std::string>;
using PanelDataMap = std::unordered_map<std::string, PanelDataTuple>;
using ItemInterfaceMap = std::map<std::string, std::variant<bool, std::string>>;

/*baseBIOSTable reference
map{attributeName,struct{attributeType,readonlyStatus,displayname,
              description,menuPath,current,default,
              array{struct{optionstring,optionvalue}}}}
*/
using BiosBaseTableItem = std::pair<
    std::string,
    std::tuple<
        std::string, bool, std::string, std::string, std::string,
        std::variant<int64_t, std::string>, std::variant<int64_t, std::string>,
        std::vector<
            std::tuple<std::string, std::variant<int64_t, std::string>>>>>;
using BiosBaseTable = std::vector<BiosBaseTableItem>;

/* SystemParameterValues reference
 std::tuple<os_ipl_type, system_operating_mode, hmc_managed, fw_boot_side,
 hyp_switch>
 */
using SystemParameterValues =
    std::tuple<std::string, std::string, std::string, std::string, std::string>;

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

} // namespace types
} // namespace panel