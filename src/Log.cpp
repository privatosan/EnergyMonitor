#include "Log.h"

#include <iomanip>
#include <chrono>

Log::Log(LogLevel type)
{
    m_logLevel = type;
    operator << ("[");
    operator << (type);
    operator << ("] ");

    std::time_t t = std::time(nullptr);
    std::tm tm = *std::localtime(&t);
#if __GNUC__ >= 5
    operator << (std::put_time(&tm, "%F %T"));
#else
    char buffer[48];
    if(0 < strftime(buffer, sizeof(buffer), "%F %T", &tm))
        operator << (buffer);
#endif
    operator << (" ");
}

Log::~Log()
{
    if (m_output)
        std::cout << std::endl;
}

Log& Log::operator<<(std::ostream& (*func)(std::ostream&))
{
    if (m_logLevel >= Options::getInstance().logLevel())
    {
        func(std::cout);
        m_output = true;
    }

    return *this;
}
