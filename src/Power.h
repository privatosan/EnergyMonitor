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
#ifdef WIRINGPI
    std::vector<int> m_fds;
#endif

    std::vector<std::unique_ptr<ChannelSum>> m_channels;

    std::unique_ptr<ChannelAD> m_voltageChannel;
    std::vector<std::unique_ptr<ChannelAD>> m_currentChannels;

    void update(std::unique_ptr<ChannelAD> &channel);

    virtual void preStart();
    virtual void postStop();
    virtual void threadFunction();
};

#endif // POWER_H
