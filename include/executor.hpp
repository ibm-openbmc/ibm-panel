#pragma once

#include "transport.hpp"
#include "types.hpp"

#include <memory>

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
    Executor(std::shared_ptr<Transport> transportObj) :
        transportObj(transportObj)
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
     * @brief An api of check if OS IPL type is enabled.
     * It is a helper function for functionality 01. Display needs to be
     * modified depending upon if OS IPL type is enabled or disabled.
     * @return OS IPL type enabled/disbaled.
     */
    bool isOSIPLTypeEnabled() const;

    /*Transport class object*/
    std::shared_ptr<Transport> transportObj;

}; // class Executor
} // namespace panel
