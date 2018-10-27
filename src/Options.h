#ifndef OPTIONS_H
#define OPTIONS_H

#include <chrono>

class Options
{
public:
    static Options &getInstance();

    void parseCmdLine(int argc, char * const *argv);

    bool verbose() const
    {
        return m_verbose;
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

    bool m_verbose;
    std::chrono::seconds m_updatePeriod;
};

#endif // OPTIONS_H
