#ifndef SOLAR_H
#define SOLAR_H

#include "BackgroundTask.h"
#include "Channel.h"
#include "SolarMax.h"

#include <time.h>

#include <vector>
#include <list>

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

    SolarMax::Value *daily() const
    {
        return m_value.get();
    }

private:
    uint32_t m_address;
    std::unique_ptr<SolarMax::Value> m_value;
    std::unique_ptr<SolarMax::Value> m_daiy;
};

class Solar : public BackgroundTask
{
public:
    Solar();
    virtual ~Solar();

private:
    std::vector<std::vector<std::unique_ptr<ChannelConverter>>> m_channelsConverter;

    std::vector<std::unique_ptr<ChannelSum>> m_channelSolar;

    bool m_statChanged;     // set if statistics changed

    // statistic
    class Stat
    {
    public:
        Stat()
            : m_day(0)
            , m_month(0)
            , m_year(0)
        {
        }

        bool sameDay(uint32_t day, uint32_t month, uint32_t year)
        {
            return ((day == m_day) && (month == m_month) && (year == m_year));
        }
        bool sameMonth(uint32_t month, uint32_t year)
        {
            return ((month == m_month) && (year == m_year));
        }

        uint32_t m_day;
        uint32_t m_month;
        uint32_t m_year;
        std::vector<uint32_t> m_values;
    };

    std::list<Stat> m_days;
    std::list<Stat> m_months;
    std::list<Stat> m_years;

    uint32_t m_max;
    uint32_t m_maxHour;
    uint32_t m_maxMinute;

    void getStat(const std::string file, const std::string start, std::list<Stat> &stats) const;
    void putStat(const std::string file, const std::string start, const std::list<Stat> &stats) const;

    void updateStat();
    void readStat();
    void writeStat();

    virtual void preStart();
    virtual void postStop();
    virtual void threadFunction();
};

#endif // SOLAR_H
