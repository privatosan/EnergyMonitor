#include "SignalHandler.h"
#include "Solar.h"
#include "Power.h"
#include "Options.h"
#include "Log.h"
#include "Server.h"

#include <unistd.h>

int main(int argc, char * const *argv)
{
    std::unique_ptr<Solar> solar;
    std::unique_ptr<Power> power;

    try
    {
        Options::getInstance().parseCmdLine(argc, argv);

        // run solar tracking in the background
        solar.reset(new Solar);
        solar->start();

        // run power tracking in the background
        power.reset(new Power);
        power->start();

        // wait for exit signal
        SignalHandler signalHandler;
        while(!signalHandler.gotExitSignal())
        {
            sleep(1);
        }
    }
    catch (std::exception &er)
    {
        Log(ERROR) << er.what();
    }

    if (power)
        power->stop();
    if (solar)
        solar->stop();

    Server::getInstance().shutdown();

    return 0;
}
