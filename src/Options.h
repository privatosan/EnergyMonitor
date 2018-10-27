#ifndef OPTIONS_H
#define OPTIONS_H

#include <chrono>
#include <ostream>

enum LogLevel
{
    DEBUG,
    INFO,
    WARN,
    ERROR
};

std::ostream& operator << (std::ostream& os, const LogLevel& logLevel);

class Options
{
public:
    static Options &getInstance();

    void parseCmdLine(int argc, char * const *argv);

    LogLevel logLevel() const
    {
        return m_logLevel;
    }

    std::chrono::seconds updatePeriod() const
    {
        return m_updatePeriod;
    }

private:
    // this is a singleton, hide copy constructor etc.
    Options(const Options&);
    Options& operator=(const Options&);
    // a singleton constructor/destructor is not thread safe and need to be empty
    Options() { };
    ~Options() { };

    void initialize();

    LogLevel m_logLevel;
    std::chrono::seconds m_updatePeriod;
};

#endif // OPTIONS_H
