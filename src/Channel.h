#ifndef CHANNEL_H
#define CHANNEL_H

#include "Command.h"
#include "Util.h"

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

    timeValueMs timestamp() const
    {
        return m_timestamp;
    }

private:
    std::string m_name;

    timeValueMs m_timestamp;
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

    virtual void set(float value)
    {
        Channel::set((value + m_offset) * m_factor);
    }

    uint32_t chipID() const
    {
        return m_chipID;
    }

private:
    uint32_t m_chipID;
    uint32_t m_channelID;
    float m_offset;
    float m_factor;
};

class ChannelConverter : public Channel
{
public:
    ChannelConverter(std::string name, uint32_t address)
        : Channel(name)
        , m_address(address)
    {
    }

    uint32_t address() const
    {
        return m_address;
    }

private:
    uint32_t m_address;
};

class ChannelSum : public Channel
{
public:
    ChannelSum(std::string name, const std::vector<std::unique_ptr<Channel>> &channels)
        : Channel(name)
        , m_channels(channels)
    {
    }

    void update()
    {
        float value = 0.f;
        for (auto &&channel : m_channels)
            value += channel->value();
        set(value);
    }

private:
    const std::vector<std::unique_ptr<Channel>> &m_channels;
};

#endif // CHANNEL_H
