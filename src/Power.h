#ifndef POWER_H
#define POWER_H

#include "BackgroundTask.h"
#include "Channel.h"

class Power : public BackgroundTask
{
public:
    Power();
    virtual ~Power();

private:
    std::vector<std::unique_ptr<Channel>> m_channels;

    std::unique_ptr<ChannelAD> m_voltageChannel;
    std::vector<std::unique_ptr<ChannelAD>> m_currentChannels;

    void update();

    virtual void preStart();
    virtual void postStop();
    virtual void threadFunction();
};

#endif // POWER_H
