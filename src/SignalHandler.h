#ifndef SIGNALHANDLER_H
#define SIGNALHANDLER_H

class SignalHandler
{
public:
    SignalHandler();
    ~SignalHandler();

    static bool gotExitSignal();

private:
    static bool m_gotExitSignal;

    static void exitSignalHandler(int ignored);
};

#endif