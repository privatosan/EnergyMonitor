#include "Options.h"

#include <mutex>
#include <iostream>

#include <getopt.h>
#include <stdlib.h>
#include <libgen.h>

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
    m_verbose = false;
    m_updatePeriod = std::chrono::seconds(10);
}

void Options::parseCmdLine(int argc, char * const *argv)
{
    enum
    {
        OPTION_HELP,
        OPTION_VERBOSE,
        OPTION_UPDATE_PERIOD,
    };

    static struct option options[] =
    {
        { "help",           no_argument,       0, 'h' },
        { "verbose",        no_argument,       0, 'v' },
        { "updateperiod",   required_argument, 0, OPTION_UPDATE_PERIOD },
        { 0,                0,                 0,  0  }
    };

    // parse the command line options
    int32_t valInt;
    while (1)
    {
        int optionIndex = 0;

        int c = getopt_long(argc, argv, "hv", options, &optionIndex);
        if (c == -1)
            break;

        switch (c)
        {
        case 'h':
            std::cout <<
                "Usage: " << basename(argv[0]) << " [OPTION]...\n" <<
                "  -v, --verbose\n"
                "    Verbose output\n"
                "  --updateperiod=PERIOD\n" <<
                "    Sets the update period to PERIOD seconds\n";
            exit(EXIT_SUCCESS);
        case 'v':
            m_verbose = true;
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
