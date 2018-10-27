#include "Power.h"
#include "Post.h"

#ifdef BCM2835
#include <bcm2835/bcm2835.h>
#elif defined(WIRINGPI)
#include <wiringPi.h>
#include <wiringPiSPI.h>
#endif

#include <cmath>
#include <iostream>
#include <iomanip>

#include <unistd.h>

static const float REFERENCE_VOLTAGE = 3.2986f; // measured
// AD inputs are offset by this voltage so that DC voltages can be measured
static const float ADC_OFFSET_VOLTAGE = 1.6488f; // measured
static const float CT_AMPERE_PER_VOLT = 30.f;
static const float CT_AMPERE_PER_VOLT_PEAK = CT_AMPERE_PER_VOLT * std::sqrt(2.f);
static const uint32_t ADC_BITS = 10;
static const uint32_t ADC_MASK = (1 << ADC_BITS) - 1;
// zero point (ADC input connected to ADC_OFFSET_VOLTAGE)
static const float ADC_OFFSET_0_0 = (517.0f / ADC_MASK); // measured
static const float ADC_OFFSET_1_7 = (509.0f / ADC_MASK); // measured
static const float LINE_VOLTAGE = 230.0f;
static const float LINE_VOLTAGE_PEAK = LINE_VOLTAGE * std::sqrt(2.f);
static const float LINE_FREQUENCY = 50.0f;
static const timeValueUs LINE_PERIOD_TIME_US = 1000000 / LINE_FREQUENCY;
// transfomer ratio between primary and secondary
static const float TRANSFORMER_RATIO = (230.0f / 12.50f); // measured
// ratio between input to transfomer and input to AD
static const float TRANSFORMER_LINE_VOLTAGE_RATIO = (228.0f / 0.962f); // measured
static const double PI = std::acos(-1.f);

// how many periods to read when calculating the power
static const uint32_t PERIODS_TO_READ = 5;

#ifndef RPI
#ifdef BCM2835
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
#endif

Power::Power()
{
    m_channels.push_back(std::unique_ptr<ChannelSum>(new ChannelSum("use")));

    m_currentChannels.push_back(std::unique_ptr<ChannelAD>(new ChannelAD("House_w0", 0, 0, -ADC_OFFSET_0_0, REFERENCE_VOLTAGE * CT_AMPERE_PER_VOLT_PEAK)));
    m_channels[0]->add(m_currentChannels.back().get());
#if 0
    m_currentChannels.push_back(std::unique_ptr<ChannelAD>(new ChannelAD("House_w1", 0, 1, -ADC_OFFSET, REFERENCE_VOLTAGE * CT_AMPERE_PER_VOLT_PEAK)));
    m_channels[0]->add(m_currentChannels.back().get());
    m_currentChannels.push_back(std::unique_ptr<ChannelAD>(new ChannelAD("House_w2", 0, 2, -ADC_OFFSET, REFERENCE_VOLTAGE * CT_AMPERE_PER_VOLT_PEAK)));
    m_channels[0]->add(m_currentChannels.back().get());
    m_currentChannels.push_back(std::unique_ptr<ChannelAD>(new ChannelAD("House_w7", 0, 7, -ADC_OFFSET, REFERENCE_VOLTAGE * CT_AMPERE_PER_VOLT_PEAK)));
    m_channels[0]->add(m_currentChannels.back().get());
#endif

    m_voltageChannel.reset(new ChannelAD("L1", 1, 7, -ADC_OFFSET_1_7, REFERENCE_VOLTAGE * TRANSFORMER_LINE_VOLTAGE_RATIO));
    // vref * (R1+R2) / R2 * Vprim / Vsec
    //m_voltageChannel.reset(new ChannelAD("L1", 1, 7, -ADC_OFFSET_1_7, REFERENCE_VOLTAGE * ((120.f + 10.f) / 10.f) * LINE_VOLTAGE_PEAK / TRANSFORMER_RATIO));
    //m_voltageChannel.reset(new ChannelAD("L1", 1, 7, -ADC_OFFSET_1_7, LINE_VOLTAGE_PEAK / ((518.f - 252.f) / 1023.f)));
}

Power::~Power()
{
}

static void read(uint32_t chipID, std::vector<Command> &cmds, std::vector<float> &values,
    std::vector<timeValueUs> &times)
{
#ifdef RPI
#ifdef BCM2835
    static const bcm2835SPIChipSelect cs[] =
    {
        BCM2835_SPI_CS0,
        BCM2835_SPI_CS1
    };

    //Log(DEBUG) << "Select chip " << chipID;
    bcm2835_spi_chipSelect(cs[chipID]);
#endif // BCM2835

    const size_t commandSize = sizeof(Command::m_sequence.m_data);
    unsigned char reply[commandSize];

    for (auto&& cmd : cmds)
    {
//Log(DEBUG) << std::hex << std::setw(2) << (uint32_t)cmd.m_sequence.m_data[0] << std::setw(2) << (uint32_t)cmd.m_sequence.m_data[1] << std::setw(2) << (uint32_t)cmd.m_sequence.m_data[2] << std::dec;
#ifdef BCM2835
        bcm2835_spi_transfernb(reinterpret_cast<char*>(cmd.m_sequence.m_data),
            reinterpret_cast<char*>(reply), commandSize);
#elif defined(WIRINGPI)
        memcpy(reply, cmd.m_sequence.m_data, commandSize);
        wiringPiSPIDataRW(chipID, reply, commandSize);
#endif // BCM2835
        times.push_back(time());

        uint32_t value = 0;
        for (size_t i = 0; i < commandSize; ++i)
        {
            value <<= 8;
            value += reply[i];
        }
//Log(DEBUG) << (uint32_t)cmd.m_sequence.m_bitfield.m_channel << ": " << value;

        // mask out undefined bits
        value &= ADC_MASK;

        float normalized = (float)value / (float)ADC_MASK;
        values.push_back(normalized);
    }
#else // RPI
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
                 LINE_VOLTAGE_PEAK / ((120.f + 10.f) / 10.f) * TRANSFORMER_RATIO
            },
        };

        float value = wave * factor[chipID][cmd.m_sequence.m_bitfield.m_channel];
        // convert to -.5 ... .5
        value /= REFERENCE_VOLTAGE / 2.f;
        // and to 0 ... 1
        value += 0.5f;
        values.push_back(value);
    }
#endif
}

void Power::update()
{
    std::vector<std::vector<Command>> cmds;

    for (auto&& channel : m_currentChannels)
    {
        const uint32_t chipID = channel->chipID();
        if (chipID >= cmds.size())
            cmds.resize(chipID + 1);
        cmds[chipID].push_back(channel->command());
        channel->clearSamples();
    }

    const uint32_t chipID = m_voltageChannel->chipID();
    if (chipID >= cmds.size())
        cmds.resize(chipID + 1);
    cmds[chipID].push_back(m_voltageChannel->command());
    m_voltageChannel->clearSamples();

    std::vector<std::vector<float>> values(cmds.size());
    std::vector<std::vector<timeValueUs>> times(cmds.size());
    std::vector<size_t> indices(cmds.size());

    const timeValueUs sampleTimeUs = 1000000 / LINE_FREQUENCY * PERIODS_TO_READ;
    timeValueUs curTimeUs;
    auto startTimeUs = time();

    do
    {
        curTimeUs = time();

        for (auto&& value: values)
            value.clear();
        for (auto&& time: times)
            time.clear();

        uint32_t chipID = 0;
        for (auto&& chipCmd : cmds)
        {
            read(chipID, chipCmd, values[chipID], times[chipID]);
            ++chipID;
        }

        for (auto&& index : indices)
            index = 0;

        chipID = m_voltageChannel->chipID();
        m_voltageChannel->setSample(times[chipID][indices[chipID]],
            values[chipID][indices[chipID]]);
        indices[chipID]++;

        for (auto&& channel : m_currentChannels)
        {
            chipID = channel->chipID();
            channel->setSample(times[chipID][indices[chipID]],
                values[chipID][indices[chipID]]);
            indices[chipID]++;
        }

#if 0
        // limit to 100 samples per period
        const timeValueUs diffTimeUs = time() - curTimeUs;
        if (diffTimeUs < LINE_PERIOD_TIME_US / 100)
            usleep(LINE_PERIOD_TIME_US / 100 - diffTimeUs);
#endif
    } while (curTimeUs - startTimeUs < sampleTimeUs);

    // find zero-crossing
    startTimeUs = 0;
    auto prevValue = m_voltageChannel->value(0);
    timeValueUs endTimeUs = 0;
    uint32_t halfPeriod = 0;
    uint32_t periods = 0;
    for (size_t index = 1; index < m_voltageChannel->sampleCount(); ++index)
    {
        const auto value = m_voltageChannel->value(index);
        if (value * prevValue < 0.f)
        {
            const auto timeUs = m_voltageChannel->sampleTime(index);
            if (startTimeUs == 0)
            {
                startTimeUs = timeUs;
Log(DEBUG) << startTimeUs;
            }
            else
            {
                ++halfPeriod;
                if ((halfPeriod & 1) == 0)
                {
                    endTimeUs = timeUs;
Log(DEBUG) << endTimeUs;
                    ++periods;
                }
            }
        }
        prevValue = value;
    }

    Log(DEBUG) << "Found " << periods << " periods, frequency " << periods / ((endTimeUs - startTimeUs) / 1000000.f) << " Hz";

    // calculate power
    for (auto&& channel : m_currentChannels)
    {
        Log(DEBUG) << "Samples " << channel->sampleCount();

        float p = 0.f;
        float i = 0.f;
        float u = 0.f;
float imin = 10.f, imax = -10.f;
float umin = 400.f, umax = -400.f;
        timeValueUs startSampleTimeUs = 0;
        timeValueUs endSampleTimeUs = 0;
        for (size_t index = 0; index < channel->sampleCount() - 1; ++index)
        {
            const timeValueUs time = channel->sampleTime(index);
            if (time < startTimeUs)
                continue;
            if (startSampleTimeUs == 0)
                startSampleTimeUs = time;
            const timeValueUs nextTime = channel->sampleTime(index + 1);
            if (nextTime > endTimeUs)
            {
                break;
            }
            endSampleTimeUs = nextTime;
            const float current = channel->value(index);
imin = std::min(imin, current);
imax = std::max(imax, current);
            const float voltage = m_voltageChannel->sampleAtTime(time);
umin = std::min(umin, voltage);
umax = std::max(umax, voltage);

//Log(WARN) << "U\t" << voltage << "\tI\t" << current;
            i += current * current * (nextTime - time);
            u += voltage * voltage * (nextTime - time);
            p += (voltage * current) * (nextTime - time);
        }

Log(DEBUG) << "I min/max " << imin << " " << imax;
Log(DEBUG) << "U min/max " << umin << " " << umax;
Log(DEBUG) << startSampleTimeUs;
Log(DEBUG) << endSampleTimeUs;

        const float invTimeRangeUs = 1.f / (float)(endSampleTimeUs - startSampleTimeUs);
        p *= invTimeRangeUs;
        u *= invTimeRangeUs;
        u = std::sqrt(u);
        i *= invTimeRangeUs;
        i = std::sqrt(i);
        Log(DEBUG) << "U " << u << " I " << i << " P " << p << std::endl;
        channel->set(p);
    }
}

void Power::preStart()
{
#ifdef BCM2835
    if (!bcm2835_init())
        throw std::runtime_error("Failed to init BCM 2835");

    if (!bcm2835_spi_begin())
        throw std::runtime_error("bcm2835_spi_begin() failed");

    bcm2835_spi_setBitOrder(BCM2835_SPI_BIT_ORDER_MSBFIRST);
    bcm2835_spi_setDataMode(BCM2835_SPI_MODE0); //Data comes in on falling edge
    bcm2835_spi_setClockDivider(BCM2835_SPI_CLOCK_DIVIDER_4096); // 19.2MHz / 32 = 600kHz
    bcm2835_spi_setChipSelectPolarity(BCM2835_SPI_CS0, LOW);
    bcm2835_spi_setChipSelectPolarity(BCM2835_SPI_CS1, LOW);
#elif defined(WIRINGPI)
    const int speed = 1 * 1000 * 1000;
    if (wiringPiSetup() == -1)
        throw std::runtime_error("Failed to setup WiringPI");
    int fd;
    fd = wiringPiSPISetup(0, speed);
    if (fd == -1)
        throw std::runtime_error("Failed to setup SPI channel 0");
    m_fds.push_back(fd);
    fd = wiringPiSPISetup(1, speed);
    if (fd == -1)
        throw std::runtime_error("Failed to setup SPI channel 1");
    m_fds.push_back(fd);
#endif
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

//    post(channels);

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
#ifdef BCM2835
    bcm2835_spi_end();
    if (!bcm2835_close())
        throw std::runtime_error("bcm2835_close() failed");
#elif defined(WIRINGPI)
    for (auto &&fd : m_fds)
        close(fd);
    m_fds.clear();
#endif
}
