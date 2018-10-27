#ifndef CHANNEL_H
#define CHANNEL_H

#include "Command.h"
#include "Util.h"
#include "Log.h"

#include <string>
#include <vector>
#include <memory>

class Channel
{
public:
    Channel(std::string name)
        : m_name(name)
        , m_timestamp(0)
        , m_value(0.f)
    {
    }
    virtual ~Channel()
    {
    }

    virtual void set(float value)
    {
        m_value = value;
        m_timestamp = time();
    }

    const std::string& name() const
    {
        return m_name;
    }

    float value() const
    {
        return m_value;
    }

    timeValueUs timestamp() const
    {
        return m_timestamp;
    }

private:
    std::string m_name;

    timeValueUs m_timestamp;
    float m_value;
};

class ChannelAD : public Channel
{
public:
    ChannelAD(std::string name, uint32_t chipID, uint32_t channelID, float offset, float factor)
        : Channel(name)
        , m_chipID(chipID)
        , m_channelID(channelID)
        , m_offset(offset)
        , m_factor(factor)
    {
    }

    Command command()
    {
        return Command(m_channelID);
    }

    uint32_t chipID() const
    {
        return m_chipID;
    }

    void setSample(timeValueUs time, float value)
    {
        m_values.push_back(TimeValue(time, (value + m_offset) * m_factor));
    }

    size_t sampleCount() const
    {
        return m_values.size();
    }

    timeValueUs sampleTime(size_t index) const
    {
        return m_values[index].m_time;
    }

    float sampleAtTime(timeValueUs time) const
    {
        size_t index = 0;

        while ((index != m_values.size()) && (time > m_values[index].m_time))
            ++index;

        size_t before = std::min(index, m_values.size() - 1);
        size_t after = std::min(before + 1, m_values.size() - 1);

        float value = m_values[before].m_value;
        if (m_values[after].m_time != m_values[before].m_time)
        {
            const float factor = (time - m_values[before].m_time) / (m_values[after].m_time - m_values[before].m_time);
            value += (m_values[after].m_value - m_values[before].m_value) * factor;
        }

        return value;
    }

    void clearSamples()
    {
        m_values.clear();
    }

private:
    struct TimeValue
    {
        TimeValue(timeValueUs time, float value)
            : m_time(time)
            , m_value(value)
        {
        }
        timeValueUs m_time;
        float m_value;
    };
    std::vector<TimeValue> m_values;

    uint32_t m_chipID;
    uint32_t m_channelID;
    float m_offset;
    float m_factor;
};

class ChannelSum : public Channel
{
public:
    ChannelSum(std::string name)
        : Channel(name)
    {
    }

    void add(const Channel *channel)
    {
        m_channels.push_back(channel);
    }

    void update()
    {
        float value = 0.f;
        for (auto &&channel : m_channels)
            value += channel->value();
        set(value);
    }

private:
    std::vector<const Channel *> m_channels;
};

#endif // CHANNEL_H
