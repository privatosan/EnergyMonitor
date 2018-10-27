#ifndef LOG_H
#define LOG_H

#include "Options.h"

#include <iostream>

class Log
{
public:
    Log(LogLevel type);
    ~Log();

    template<class T>
    Log &operator<<(const T &msg)
    {
        if (m_logLevel >= Options::getInstance().logLevel())
        {
            std::cout << msg;
            m_output = true;
        }
        return *this;
    }

    Log &operator<<(std::ostream& (*func)(std::ostream&));

private:
    Log() { }

    bool m_output = false;
    LogLevel m_logLevel = DEBUG;
};

#endif // LOG_H
