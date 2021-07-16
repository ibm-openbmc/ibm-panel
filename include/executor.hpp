#pragma once

#include "transport.hpp"
#include "types.hpp"

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
     * @brief An api to execute function 11.
     */
    void execute11();

    /**
     * @brief An api of check if OS IPL type is enabled.
     * It is a helper function for functionality 01. Display needs to be
     * modified depending upon if OS IPL type is enabled or disabled.
     * @return OS IPL type enabled/disbaled.
     */
    bool isOSIPLTypeEnabled() const;

    /*Transport class object*/
    std::shared_ptr<Transport> transport;

    /* Event object path of last logged PEL */
    sdbusplus::message::object_path pelEventPath{};

}; // class Executor
} // namespace panel
