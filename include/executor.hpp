#pragma once

#include "transport.hpp"
#include "types.hpp"

#include <deque>
#include <memory>
#include <sdbusplus/asio/object_server.hpp>
#include <sdbusplus/message/native_types.hpp>

namespace panel
{
/**
 * @brief A class to execute panel functionalities.
 */
class Executor
{
  public:
    /* Deleted Api's*/
    Executor(const Executor&) = delete;
    Executor& operator=(const Executor&) = delete;
    Executor(Executor&&) = delete;

    /* Destructor */
    ~Executor() = default;

    /**
     * @brief Constructor
     * @param[in] transport - Pointer to transport class.
     * @param[in] iface - Pointer to Panel dbus interface.
     * @param[in] io - reference to io context class.
     */
    Executor(std::shared_ptr<Transport> transport,
             std::shared_ptr<sdbusplus::asio::dbus_interface>& iface,
             std::shared_ptr<boost::asio::io_context>& io) :
        transport(transport),
        iface(iface), io_context(io)
    {
    }

    /**
     * @brief An api to execute a given function/sub-function.
     *
     * For functionalities having sub-functions like function 30, passing the
     * sub-function number is required.
     * List is required as parameter as some function like 02 needs a list of
     * sub-states to execute.
     *
     * @param[in] funcNumber - function number to execute.
     * @param[in] subFuncNumber - Sub function number to be executed.
     */
    void executeFunction(const types::FunctionNumber funcNumber,
                         const types::FunctionalityList& subFuncNumber);

    /**
     * @brief Api to store callout list of last PEL.
     * @param[in] callOuts - list of callouts.
     */
    inline void
        pelCallOutList(const std::tuple<std::string, std::string>& callOuts)
    {
        callOutList.push_back(callOuts);
    }

    /**
     * @brief An api to store event id and object path of last 25 PELs.
     * This will be required by function 64 sub functions.
     *
     * @param[in] objPath - Pel path corresponding to the event id.
     * @param[in] pelEventId - Value of eventId property of a given PEL.
     */
    void storePel(const std::string& objPath, const std::string& pelEventId);

    /**
     * @brief An api to delete pel from stored list.
     *
     * @param objPath - Object path of the pel to be deleted.
     */
    void deletePel(const std::string& objPath);

    /**
     * @brief An api to store latest SRC and Hexwords.
     * This api will receive string consisting SRC and Hexwords and store them
     * to be used in function 11, 12 and 13.
     *
     * @param[in] srcAndHexwords- String consisting of SRC and hex words.
     */
    inline void storeSRCAndHexwords(const std::string& srcAndHexwords)
    {
        latestSrcAndHexwords = srcAndHexwords;
    }

    /**
     * @brief An api to return count of Pel stored.
     * This count is required to enable/disable sub functions by state manager
     * w.r.t function 64.
     * @return The current count of stored PELs.
     */
    inline uint8_t getStoredPelCount() const
    {
        return pelList.size();
    }

    /**
     * @brief An api to store last 25 IPL SRCs.
     * @param[in] progressCode - The progress code to store.
     */
    void storeIPLSRC(const std::string& progressCode);

    /**
     * @brief An api to get count of IPL SRCs.
     * It will be required to enable/disable sub functions for function 63 based
     * on SRC count.
     * @return The current count of stored progress code SRCs.
     */
    inline uint8_t getIPLSRCCount() const
    {
        return iplSrcs.size();
    }

    /**
     * @brief An api to get state of service switch 1.
     * It is required to enable CE mode by state manager.
     *
     * @return state of service switch 1.
     */
    inline bool getServiceSwitch1State() const
    {
        return serviceSwitch1State;
    }

    /**
     * @brief API which sets OS IPL mode.
     * This api is called whenever there is a property change signal occurs for
     * OSIPLMode com.ibm.panel interface's property and the current OS IPL mode
     * value is stored in Executor.
     *
     * @param[in] osIPLModeState - current osIPLMode
     */
    inline void setOSIPLMode(const bool osIPLModeState)
    {
        osIplMode = osIPLModeState;
    }

  private:
    /**
     * @brief An api to execute functionality 20
     */
    void execute20();

    /**
     * @brief An api to execute functionality 01.
     */
    void execute01();

    /**
     * @brief An api to execute function 04.
     */
    void execute04();

    /**
     * @brief An api to execute function 02.
     */
    void execute02(const types::FunctionalityList& subFuncNumber);

    /**
     * @brief An api to execute function 03.
     */
    void execute03();

    /**
     * @brief An api to execute function 08.
     */
    void execute08();

    /**
     * @brief An api to execute function 11.
     */
    void execute11();

    /**
     * @brief An api to execute function 12.
     */
    void execute12();

    /**
     * @brief An api to execute function 13.
     */
    void execute13();

    /**
     * @brief An api to execute function 14 to 19.
     * @param[in] funcNumber - function to execute.
     */
    void execute14to19(const types::FunctionNumber funcNumber);

    /**
     * @brief An api to execute function 25.
     * Function 25 along with function 26 is usd to enable CE mode functions in
     * panel.
     */
    void execute25();

    /**
     * @brief An api to execute function 26.
     * Function 26 when executed after function 25 enables CE mode in panel. If
     * function 25 has not been executed before function 26, then execution of
     * this function will fail and FF will be displayed.
     */
    void execute26();

    /**
     * @brief An api to execute function 63.
     * @param[in] subFuncNumber - Sub function to execute.
     */
    void execute63(const types::FunctionNumber subFuncNumber);

    /**
     * @brief An api to execute function 64.
     * @param[in] subFuncNumber - Sub function to execute.
     */
    void execute64(const types::FunctionNumber subFuncNumber);

    /**
     * @brief An api to execute function 73.
     */
    void execute73();

    /**
     * @brief An api to execute function 74.
     */
    void execute74();

    /**
     * @brief Api to get PEL eventId.
     *
     * This is a helper function to get the eventId(SRC) data for the PEL
     * logged.
     */
    std::string getSrcDataForPEL();

    /** @brief API to execute function 30. */
    void execute30(const types::FunctionalityList& subFuncNumber);

    /**
     * @brief To display the execution result (function success/failure
     * (00/FF)).
     * @param[in] funcNumber - function number
     * @param[in] subFuncNumber - sub function number list
     * @param[in] result - Execution result - true:success(00) / false:failure
     * (FF)
     */
    void displayExecutionStatus(const types::FunctionNumber funcNumber,
                                const types::FunctionalityList& subFuncNumber,
                                const bool result);

    /**
     * @brief API to initiate disruptive platform system dump.
     */
    void execute42();

    /**
     * @brief API to execute function 55.
     * @param[in] subFuncNumber - Sub function vector.
     */
    void execute55(const types::FunctionalityList& subFuncNumber);

    /**
     * @brief Api to initiate service processor dump.
     * This method triggers a service processor dump when function 43 is pressed
     * in panel.
     */
    void execute43();

    /**
     * @brief API to send function number to PHYP.
     * Some of the functions(like 21,22,34,41,65-70) needs to be executed by
     * PHYP. This method sends the function number to phyp via the PldmFramework
     * api and displays 00 on a successful send and FF on a failure.
     *
     * @param[in] funcNumber - Function number.
     */
    void sendFuncNumToPhyp(const types::FunctionNumber& funcNumber);

    /*Transport class object*/
    std::shared_ptr<Transport> transport;

    /* List of resolution property added to callouts */
    std::vector<std::tuple<std::string, std::vector<std::string>>> callOutList;

    /* List of last 25 IPL SRCs. */
    std::deque<std::string> iplSrcs;

    /* Queue of last 25 PEL SRCs and their object path*/
    std::deque < std::tuple<std::string, std::string> pelQueue;

    /*State of function 25 excution. Needed for function 26*/
    bool serviceSwitch1State = false;

    /* Pointer to interface */
    std::shared_ptr<sdbusplus::asio::dbus_interface> iface;

    /* Event for timer required in function 74 */
    std::shared_ptr<boost::asio::io_context> io_context;

    /* OS IPL mode state */
    bool osIplMode = false;

    /* SRC and HEX words */
    std::string latestSrcAndHexwords;

}; // class Executor
} // namespace panel
