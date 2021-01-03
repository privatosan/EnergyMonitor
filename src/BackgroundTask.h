#ifndef BACKGROUND_TASK_H
#define BACKGROUND_TASK_H

#include <memory>
#include <thread>
#include <condition_variable>

class BackgroundTask
{
public:
    BackgroundTask(bool periodical);
    BackgroundTask() = delete;
    ~BackgroundTask();

    void start();
    void stop();

private:
    std::unique_ptr<std::thread> m_thread;

    std::condition_variable m_conditionVariable;
    std::mutex cv_mutex;
    bool m_periodical;
    bool m_stop;

    void threadLoop();

protected:
    virtual void preStart() { };
    virtual void postStop() { };
    virtual void threadFunction() = 0;
};

#endif // BACKGROUND_TASK_H
