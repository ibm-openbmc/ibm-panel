#pragma once

#include "transport.hpp"
#include "types.hpp"

#include <deque>
#include <memory>
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
     */
    Executor(std::shared_ptr<Transport> transport) : transport(transport)
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
     * @brief Api to store event path of last PEL.
     * @param[in] lastPel - Object path of last PEL.
     */
    inline void lastPELId(const sdbusplus::message::object_path& lastPel)
    {
        pelEventPath = lastPel;
    }

    /**
     * @brief Api to store callout list of last PEL.
     * @param[in] callOuts - list of callouts.
     */
    inline void pelCallOutList(const std::vector<std::string>& callOuts)
    {
        callOutList = callOuts;
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
     * @brief An api to execute function 02.
     */
    void execute02(const types::FunctionalityList& subFuncNumber);

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
     * @brief An api to execute function 63.
     * @param[in] subFuncNumber - Sub function to execute.
     */
    void execute63(const types::FunctionNumber subFuncNumber);

    /**
     * @brief Api to get PEL eventId.
     *
     * This is a helper function to get the eventId(SRC) data for the PEL
     * logged.
     */
    std::string getSrcDataForPEL();

    /**
     * @brief An api of check if OS IPL type is enabled.
     * It is a helper function for functionality 01. Display needs to be
     * modified depending upon if OS IPL type is enabled or disabled.
     * @return OS IPL type enabled/disbaled.
     */
    bool isOSIPLTypeEnabled() const;

    /** @brief API to execute function 30. */
    void execute30(const types::FunctionalityList& subFuncNumber);

    /*Transport class object*/
    std::shared_ptr<Transport> transport;

    /* Event object path of last logged PEL */
    sdbusplus::message::object_path pelEventPath{};

    /* List of resolution property added to callouts */
    std::vector<std::string> callOutList;

    /* List of last 25 IPL SRCs. */
    std::deque<std::string> iplSrcs;

}; // class Executor
} // namespace panel
