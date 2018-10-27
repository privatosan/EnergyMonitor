#ifndef SOLAR_H
#define SOLAR_H

#include <memory>
#include <thread>
#include <condition_variable>
#include <vector>

class Channel;

class Solar
{
public:
    Solar();
    ~Solar();

    void start();
    void stop();
private:
    std::vector<Channel> m_channels;

    std::unique_ptr<std::thread> m_thread;

    std::condition_variable m_conditionVariable;
    std::mutex cv_mutex;
    bool m_stop;

    void threadFunction();
};

#endif // SOLAR_H
