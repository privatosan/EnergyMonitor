#ifndef SOLAR_H
#define SOLAR_H

#include "Channel.h"
#include "SolarMax.h"

#include <time.h>

#include <memory>
#include <thread>
#include <condition_variable>
#include <vector>

class ChannelConverter : public Channel
{
public:
    ChannelConverter(std::string name, uint32_t address, SolarMax::Value *value)
        : Channel(name)
        , m_address(address)
        , m_value(value)
    {
    }

    uint32_t address() const
    {
        return m_address;
    }

    SolarMax::Value *value() const
    {
        return m_value.get();
    }

 private:
    uint32_t m_address;
    std::unique_ptr<SolarMax::Value> m_value;
};

class Solar
{
public:
    Solar();
    ~Solar();

    void start();
    void stop();
private:
    std::vector<std::vector<std::unique_ptr<ChannelConverter>>> m_channelsConverter;

    std::vector<std::unique_ptr<ChannelSum>> m_channelSolar;

    std::unique_ptr<std::thread> m_thread;

    std::condition_variable m_conditionVariable;
    std::mutex cv_mutex;
    bool m_stop;

    // statistic
    struct Stat
    {
        uint32_t m_day;
        uint32_t m_month;
        uint32_t m_year;
        std::vector<uint32_t> m_values;
    };

    std::vector<Stat> m_days;
    std::vector<Stat> m_months;
    std::vector<Stat> m_years;

    void getStat(const std::string file, const std::string start, std::vector<Stat> &stats) const;
    void putStat(const std::string file, const std::string start, const std::vector<Stat> &stats) const;

    void readStat();
    void writeStat();

    void threadFunction();
};

#endif // SOLAR_H
