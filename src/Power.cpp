#include "Power.h"
#include "Post.h"

#include <bcm2835/bcm2835.h>

#include <cmath>
#include <iostream>

#include <unistd.h>

static const float REFERENCE_VOLTAGE = 3.3f;
static const float CT_AMPERE_PER_VOLT = 30.f;
static const uint32_t ADC_BITS = 10;
static const float LINE_VOLTAGE = 230.0f;
static const float LINE_VOLTAGE_PEAK = 230.0f * std::sqrt(2.f);
static const float LINE_FREQUENCY = 50.0f;
static const double PI = std::acos(-1.f);

// how many periods to read when calculating the power
static const uint32_t PERIODS_TO_READ = 3;

#ifndef RPI
int bcm2835_init(void) { return 1; }
int bcm2835_close(void) { return 1; }
int bcm2835_spi_begin() { return 1; }
void bcm2835_spi_end() { }
void bcm2835_spi_setBitOrder(uint8_t order) { }
void bcm2835_spi_setDataMode(uint8_t mode) {}
void bcm2835_spi_setClockDivider(uint16_t divider) {}
void bcm2835_spi_setChipSelectPolarity(uint8_t cs, uint8_t active) {}
void bcm2835_spi_chipSelect(uint8_t cs) {}
void bcm2835_spi_transfernb(char* tbuf, char* rbuf, uint32_t len) { }
#endif

Power::Power()
{
    m_channels.push_back(std::unique_ptr<ChannelSum>(new ChannelSum("use")));

    m_currentChannels.push_back(std::unique_ptr<ChannelAD>(new ChannelAD("House_w0", 0, 0, -0.5f, (REFERENCE_VOLTAGE / 2.f) * CT_AMPERE_PER_VOLT)));
    m_channels[0]->add(m_currentChannels.back().get());
    m_currentChannels.push_back(std::unique_ptr<ChannelAD>(new ChannelAD("House_w1", 0, 1, -0.5f, (REFERENCE_VOLTAGE / 2.f) * CT_AMPERE_PER_VOLT)));
    m_channels[0]->add(m_currentChannels.back().get());
    m_currentChannels.push_back(std::unique_ptr<ChannelAD>(new ChannelAD("House_w2", 0, 2, -0.5f, (REFERENCE_VOLTAGE / 2.f) * CT_AMPERE_PER_VOLT)));
    m_channels[0]->add(m_currentChannels.back().get());

    // vref / 2 * (R1+R2) / R2 * Vprim / Vsec
    m_voltageChannel.reset(new ChannelAD("L1", 1, 7, -0.5f, (REFERENCE_VOLTAGE / 2.f) * ((110.f / 10.f * LINE_VOLTAGE / 9.f))));
}

Power::~Power()
{
}

static void read(uint32_t chipID, std::vector<Command> &cmds, std::vector<float> &result)
{
#ifdef RPI
    static const bcm2835SPIChipSelect cs[] =
    {
        BCM2835_SPI_CS0,
        BCM2835_SPI_CS1
    };
    bcm2835_spi_chipSelect(cs[chipID]);

    std::vector<char> cmdSequence;

    for (auto&& cmd : cmds)
    {
        for (size_t i = 0; i < sizeof(cmd.m_sequence.m_data); ++i)
            cmdSequence.push_back(cmd.m_sequence.m_data[i]);
    }

    std::vector<char> reply;

    reply.resize(cmdSequence.size());

    bcm2835_spi_transfernb(cmdSequence.data(), reply.data(), cmdSequence.size());

    size_t index = 0;
    while (index < reply.size())
    {
        for (size_t i = 0; i < sizeof(Command::m_sequence.m_data); ++i)
        {
            value <<= 8;
            value += reply[index++];
        }
        float normalized = (float)value / (float)((1 << ADC_BITS) - 1);
        result.push_back(normalized);
    }
#else
    static timeValueUs startTime = 0;
    if (!startTime)
        startTime = time();
    const timeValueUs curTime = time() - startTime;
    const float wave = std::sin(std::fmod((float)curTime / 1000000.f / (1.f / LINE_FREQUENCY), 1.f) * 2.0f * PI);
    for (auto&& cmd : cmds)
    {
        // factor to calculate voltage from input signal
        static const float factor[2][8] =
        {
            {
                 0.9f / CT_AMPERE_PER_VOLT, // 146.37 W
                 0.5f / CT_AMPERE_PER_VOLT, // 81.32 W
                 1.6f / CT_AMPERE_PER_VOLT, // 260.22 W
                 0.8f / CT_AMPERE_PER_VOLT,
                 0.1f / CT_AMPERE_PER_VOLT,
                 0.2f / CT_AMPERE_PER_VOLT,
                 0.3f / CT_AMPERE_PER_VOLT,
                 0.4f / CT_AMPERE_PER_VOLT,
            },
            {
                 1.f / CT_AMPERE_PER_VOLT,
                 2.f / CT_AMPERE_PER_VOLT,
                 3.f / CT_AMPERE_PER_VOLT,
                 4.f / CT_AMPERE_PER_VOLT,
                 5.f / CT_AMPERE_PER_VOLT,
                 6.f / CT_AMPERE_PER_VOLT,
                 7.f / CT_AMPERE_PER_VOLT, // 1138.44
                 // voltage channel
                 LINE_VOLTAGE_PEAK / (110.f / 10.f * LINE_VOLTAGE / 9.f)
            },
        };

        float value = wave * factor[chipID][cmd.m_sequence.m_bitfield.m_channel];
        // convert to -.5 ... .5
        value /= REFERENCE_VOLTAGE / 2.f;
        // and to 0 ... 1
        value += 0.5f;
        result.push_back(value);
    }
#endif
}

void Power::update()
{
    std::vector<std::vector<Command>> cmds;

    const uint32_t chipID = m_voltageChannel->chipID();
    if (chipID >= cmds.size())
        cmds.resize(chipID + 1);
    cmds[chipID].push_back(m_voltageChannel->command());
    m_voltageChannel->clearSamples();

    for (auto&& channel : m_currentChannels)
    {
        const uint32_t chipID = channel->chipID();
        if (chipID >= cmds.size())
            cmds.resize(chipID + 1);
        cmds[chipID].push_back(channel->command());
        channel->clearSamples();
    }

    timeValueUs startTime = time();
    timeValueUs curTime;
    do
    {
        std::vector<std::vector<float>> results;

        results.resize(cmds.size());

        curTime = time();

        uint32_t chipID = 0;
        for (auto&& chipCmd : cmds)
        {
            read(chipID, chipCmd, results[chipID]);
            ++chipID;
        }

        std::vector<size_t> indices;
        indices.resize(cmds.size());
        for (auto&& index : indices)
            index = 0;

        chipID = m_voltageChannel->chipID();
        m_voltageChannel->setSample(curTime, results[chipID][indices[chipID]++]);

        for (auto&& channel : m_currentChannels)
        {
            chipID = channel->chipID();
            channel->setSample(curTime, results[chipID][indices[chipID]++]);
        }

        // limit to 100 samples per period
        const timeValueUs diffTime = time() - curTime;
        const timeValueUs periodTime = 1000000 / LINE_FREQUENCY;
        if (diffTime < periodTime / 100.f)
            usleep(periodTime / 100.f - diffTime);

    } while(curTime - startTime < 1000000 / LINE_FREQUENCY * PERIODS_TO_READ);

    // calculate power
    for (auto&& channel : m_currentChannels)
    {
        float p = 0.f;
        for (size_t index = 0; index < channel->sampleCount() - 1; ++index)
        {
            const timeValueUs time = channel->sampleTime(index);
            const timeValueUs nextTime = channel->sampleTime(index + 1);
            const float current = channel->sampleAtTime(time);
            const float voltage = m_voltageChannel->sampleAtTime(time);

            p += (voltage * current) * (nextTime - time);
        }
        const timeValueUs timeRange = channel->sampleTime(channel->sampleCount() - 1) - channel->sampleTime(0);
        p /= (float)timeRange;
        channel->set(p);
    }
}

void Power::preStart()
{
    if(!bcm2835_init())
        throw std::runtime_error("Failed to init BCM 2835");

    if (!bcm2835_spi_begin())
        throw std::runtime_error("bcm2835_spi_begin() failed");

    bcm2835_spi_setBitOrder(BCM2835_SPI_BIT_ORDER_MSBFIRST);
    bcm2835_spi_setDataMode(BCM2835_SPI_MODE0); //Data comes in on falling edge
    bcm2835_spi_setClockDivider(BCM2835_SPI_CLOCK_DIVIDER_256); //250MHz / 256 = 976.5kHz
    bcm2835_spi_setChipSelectPolarity(BCM2835_SPI_CS0, LOW);
    bcm2835_spi_setChipSelectPolarity(BCM2835_SPI_CS1, LOW);
}

void Power::threadFunction()
{
    update();

    std::vector<const Channel*> channels;
    for (auto &&channel : m_currentChannels)
    {
        channels.push_back(channel.get());
    }
    for (auto &&channel : m_channels)
    {
        channel->update();
        channels.push_back(channel.get());
    }

    post(channels);

#if 0
    bcm2835_spi_chipSelect(BCM2835_SPI_CS0);
    uint64_t start = time();
    do
    {
        Command cmd(0);
        char reply[3];

        bcm2835_spi_transfernb(cmd.m_sequence.m_data, reply, sizeof(reply));
        printf("%d, %d\n", (uint32_t)(time() - start) / 1000,
            (uint32_t)reply[1] * 0xFF + (uint32_t)reply[2]);
        usleep(500);
    } while(time() - start < 40000000);

    for (unsigned int chipSelect = 0; chipSelect < 2; ++chipSelect)
    {
        const bcm2835SPIChipSelect cs[] =
        {
            BCM2835_SPI_CS0,
            BCM2835_SPI_CS1
        };
        printf("chip %d\n", chipSelect);
        bcm2835_spi_chipSelect(cs[chipSelect]);

        for (unsigned int channel = 0; channel < 8; ++channel)
        {
            Command cmd(channel);
            char reply[3];

            printf("[%d] %02X %02X %02X -> ", channel, cmd.m_sequence.m_data[0],
                cmd.m_sequence.m_data[1], cmd.m_sequence.m_data[2]);
            bcm2835_spi_transfernb(cmd.m_sequence.m_data, reply, sizeof(reply));
            printf("%02X %02X %02X\n", reply[0], reply[1], reply[2]);
        }
    }
#endif
}

void Power::postStop()
{
    bcm2835_spi_end();
    if (!bcm2835_close())
        throw std::runtime_error("bcm2835_close() failed");
}
