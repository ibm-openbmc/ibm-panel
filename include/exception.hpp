#pragma once
#include <string>

namespace panel
{
class BaseException : public std::exception
{
  public:
    // deleted constructors
    BaseException() = delete;
    BaseException(const BaseException&) = delete;
    BaseException(BaseException&&) = delete;

    // delete overloading operators
    BaseException& operator=(const BaseException&) = delete;
    BaseException& operator=(const BaseException&&) = delete;

    // default destructor
    ~BaseException() = default;

    /** @brief Constructor
     * @param[in] msg - Error message.
     */
    explicit BaseException(const std::string& msg) : error(msg)
    {
    }

    /** @brief what method
     * This method overrides the what() method in base std::exception class.
     */
    inline const char* what() const noexcept override
    {
        return error.c_str();
    }

  private:
    std::string error;
};

class FunctionFailure : public BaseException
{
  public:
    // deleted constructors
    FunctionFailure() = delete;
    FunctionFailure(const FunctionFailure&) = delete;
    FunctionFailure(FunctionFailure&&) = delete;

    // delete overloading operators
    FunctionFailure& operator=(const FunctionFailure&) = delete;
    FunctionFailure& operator=(const FunctionFailure&&) = delete;

    // default destructor
    ~FunctionFailure() = default;

    /** @brief Constructor
     * @param[in] msg - Error message.
     */
    explicit FunctionFailure(const std::string& msg) : BaseException(msg)
    {
    }
};
} // namespace panel
