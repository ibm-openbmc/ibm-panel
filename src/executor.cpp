#include "executor.hpp"

namespace panel
{

void Executor::executeFunction(
    const panel::types::FunctionNumber funcNumber,
    const panel::types::FunctionalityList& subFuncNumber)
{
    // test output, to be removed
    std::cout << funcNumber << std::endl;
    std::cout << subFuncNumber.at(0) << std::endl;
}

void Executor::execute01()
{
    // TODO implementation
}

void Executor::execute02()
{
    // TODO implementation
}

} // namespace panel