#include "SignalHandler.h"

#include <signal.h>
#include <errno.h>
#include <stdexcept>

bool SignalHandler::m_gotExitSignal = false;

SignalHandler::SignalHandler()
{
    if (signal((int)SIGINT, SignalHandler::exitSignalHandler) == SIG_ERR)
    {
        throw std::runtime_error("Error setting up signal handler");
    }
}

SignalHandler::~SignalHandler()
{
}

bool SignalHandler::gotExitSignal()
{
    return m_gotExitSignal;
}

void SignalHandler::exitSignalHandler(int ignored)
{
    m_gotExitSignal = true;
}
