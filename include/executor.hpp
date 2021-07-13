#pragma once

#include "types.hpp"
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

    /* Constructor */
    Executor() = default;

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
    void executeFunction(const panel::types::FunctionNumber funcNumber,
                         const panel::types::FunctionalityList& subFuncNumber);

  private:
    /**
     * @brief An api to execute function 01.
     */
    void execute01();

    /**
     * @brief An api to execute function 02.
     */
    void execute02();

}; // class Executor
} // namespace panel
