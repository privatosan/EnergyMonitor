#ifndef SOLAR_H
#define SOLAR_H

#include <memory>
#include <thread>
#include <condition_variable>
#include <vector>

class ChannelConverter;
class ChannelSum;

class Solar
{
public:
    Solar();
    ~Solar();

    void start();
    void stop();
private:
    std::vector<std::unique_ptr<ChannelConverter>> m_channelsConverter;

    std::unique_ptr<ChannelSum> m_channelSolar;

    std::unique_ptr<std::thread> m_thread;

    std::condition_variable m_conditionVariable;
    std::mutex cv_mutex;
    bool m_stop;

    void threadFunction();
};

#endif // SOLAR_H
