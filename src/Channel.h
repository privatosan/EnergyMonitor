#ifndef CHANNEL_H
#define CHANNEL_H

#include "Command.h"
#include "Util.h"

#include <string>

class Channel
{
public:
    Channel(std::string name, uint32_t chipID, uint32_t channelID, float offset, float factor)
        : m_name(name)
        , m_chipID(chipID)
        , m_channelID(channelID)
        , m_offset(offset)
        , m_factor(factor)
        , m_timestamp(0)
        , m_value(0.f)
    {
    }

    Command command()
    {
        return Command(m_channelID);
    }

    void set(float value)
    {
        m_value = (value + m_offset) * m_factor;
        m_timestamp = time();
    }

    const std::string& name() const
    {
        return m_name;
    }

    uint32_t chipID() const
    {
        return m_chipID;
    }

    float value() const
    {
        return m_value;
    }

    timeValueMs timestamp() const
    {
        return m_timestamp;
    }

private:
    std::string m_name;
    uint32_t m_chipID;
    uint32_t m_channelID;
    float m_offset;
    float m_factor;

    timeValueMs m_timestamp;
    float m_value;
};

#endif // CHANNEL_H
