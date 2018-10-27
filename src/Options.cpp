#include "Options.h"

#include <mutex>
#include <iostream>
#include <sstream>

#include <getopt.h>
#include <stdlib.h>
#include <libgen.h>

std::ostream& operator << (std::ostream& os, const LogLevel& logLevel)
{
    switch(logLevel)
    {
    case DEBUG:
        os << std::string("DEBUG");
        break;
    case INFO:
        os << std::string("INFO ");
        break;
    case WARN:
        os << std::string("WARN ");
        break;
    case ERROR:
        os << std::string("ERROR");
        break;
    default:
        os.setstate(std::ios_base::failbit);
    }
    return os;
}

Options &Options::getInstance()
{
    static std::once_flag onceFlag;
    static Options instance;

    std::call_once(onceFlag, [] {
        instance.initialize();
    });

    return instance;
}

void Options::initialize()
{
    m_logLevel = ERROR;
    m_updatePeriod = std::chrono::seconds(5* 60);
}

void Options::parseCmdLine(int argc, char * const *argv)
{
    enum
    {
        OPTION_HELP,
        OPTION_LOG_LEVEL,
        OPTION_UPDATE_PERIOD,
    };

    static struct option options[] =
    {
        { "help",           no_argument,       0, 'h' },
        { "loglevel",       required_argument, 0, 'l' },
        { "updateperiod",   required_argument, 0, OPTION_UPDATE_PERIOD },
        { 0,                0,                 0,  0  }
    };

    // parse the command line options
    int32_t valInt;
    while (1)
    {
        int optionIndex = 0;

        int c = getopt_long(argc, argv, "hl:", options, &optionIndex);
        if (c == -1)
            break;

        switch (c)
        {
        case 'h':
            std::cout <<
                "Usage: " << basename(argv[0]) << " [OPTION]...\n" <<
                "  -l, --loglevel=LEVEL\n"
                "    Set the log level to LEVEL. LEVEL can be '" << DEBUG << "', '" << INFO << "', '" << WARN << "' or '" << ERROR << "'. Default " << m_logLevel << ".\n"
                "  --updateperiod=PERIOD\n" <<
                "    Sets the update period to PERIOD seconds. Default " << m_updatePeriod.count() << ".\n";
            exit(EXIT_SUCCESS);
        case 'l':
            {
                std::ostringstream s;
                s << DEBUG;
                if (std::string(optarg) == s.str())
                {
                    m_logLevel = DEBUG;
                    break;
                }
            }
            {
                std::ostringstream s;
                s << INFO;
                if (std::string(optarg) == s.str())
                {
                    m_logLevel = INFO;
                    break;
                }
            }
            {
                std::ostringstream s;
                s << WARN;
                if (std::string(optarg) == s.str())
                {
                    m_logLevel = WARN;
                    break;
                }
            }
            {
                std::ostringstream s;
                s << ERROR;
                if (std::string(optarg) == s.str())
                {
                    m_logLevel = ERROR;
                    break;
                }
            }
            throw std::runtime_error("Invalid log level");
            break;
        case OPTION_UPDATE_PERIOD:
            valInt = atoi(optarg);
            if (valInt <= 0)
                throw std::runtime_error("Invalid update period value");
            m_updatePeriod = std::chrono::seconds(valInt);
            break;
        default:
            throw std::runtime_error("Unhandled option");
        }
    }
}
