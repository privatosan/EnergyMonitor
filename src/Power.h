#ifndef POWER_H
#define POWER_H

#include "BackgroundTask.h"
#include "Channel.h"

#include <list>

class Power : public BackgroundTask
{
public:
    Power();
    virtual ~Power();

private:
#ifdef WIRINGPI
    std::vector<int> m_fds;
#endif

    std::list<std::unique_ptr<ChannelSum>> m_channels;

    std::unique_ptr<ChannelAD> m_voltageChannel;
    std::list<std::unique_ptr<ChannelAD>> m_currentChannels;

    // reference voltage
    float m_refVoltage;
    // AD inputs are offset by this voltage so that DC voltages can be measured
    float m_adcOffsetVoltage;

    void update(std::unique_ptr<ChannelAD> &channel);

    virtual void preStart();
    virtual void postStop();
    virtual void threadFunction();
};

#endif // POWER_H
