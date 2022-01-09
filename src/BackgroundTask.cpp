#include "BackgroundTask.h"
#include "Options.h"

#include "Log.h"

BackgroundTask::BackgroundTask(bool periodical)
    : m_periodical(periodical)
    , m_stop(false)
{
}

BackgroundTask::~BackgroundTask()
{
    stop();
}

void BackgroundTask::start()
{
    if (m_thread)
        throw std::runtime_error("Thread already running");

    preStart();

    m_thread.reset(new std::thread(&BackgroundTask::threadLoop, this));
}

void BackgroundTask::stop()
{
    if (!m_thread)
        return;

    {
        std::lock_guard<std::mutex> lock(cv_mutex);
        m_stop = true;
    }
    m_conditionVariable.notify_all();

    m_thread->join();
    m_thread.reset();

    postStop();
}

void BackgroundTask::threadLoop()
{
    while (!m_stop)
    {
        threadFunction();

        std::unique_lock<std::mutex> lock(cv_mutex);
        if (m_periodical)
        {
            m_conditionVariable.wait_for(lock, Options::getInstance().updatePeriod());
        }
        else
        {
            m_conditionVariable.wait(lock);
        }
    }
}
