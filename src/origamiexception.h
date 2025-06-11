#include<exception>
#include <string>

class OrigamiException
    : public std::exception
{
public:

    OrigamiException(const char* message)
        : std::exception(message) {
        m_message = message;
    }

    virtual const char* what() const throw()
    {
        return m_message;
    }

private:
    const char* m_message;
};