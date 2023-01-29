#include "Power.h"

#include "Post.h"
#include "Server.h"
#include "Settings.h"

#ifdef BCM2835
#include <bcm2835/bcm2835.h>
#elif defined(WIRINGPI)
#include <wiringPi.h>
#include <wiringPiSPI.h>
#endif

#include <sched.h>
#include <unistd.h>

#include <cmath>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <vector>

static const uint32_t ADC_BITS = 10;
static const uint32_t ADC_MASK = (1 << ADC_BITS) - 1;
// zero point (ADC input connected to 'adcOffsetVoltage')
static const float ADC_OFFSET = (511.0f / ADC_MASK);
static const float LINE_VOLTAGE = 230.0f;
static const float LINE_VOLTAGE_PEAK = LINE_VOLTAGE * std::sqrt(2.f);
static const float LINE_FREQUENCY = 50.0f;
static const timeValueUs LINE_PERIOD_TIME_US = 1000000 / LINE_FREQUENCY;
// transfomer ratio between primary and secondary
static const float TRANSFORMER_RATIO = (230.0f / 12.50f);  // measured
// ratio between input to transfomer and input to AD
static const float TRANSFORMER_LINE_VOLTAGE_RATIO = (228.0f / 0.962f);  // measured
static const double PI = std::acos(-1.f);

// calibration
static const float CAL_OFFSET_VOLTAGE = (-2.0f / ADC_MASK);  // measured
static const float CAL_FACTOR_VOLTAGE = 1.0f;
// CT sensor phase correction (7 degree)
// (see
// https://learn.openenergymonitor.org/electricity-monitoring/ct-sensors/yhdc-ct-sensor-report)
static const timeValueUs CAL_PHASE_CORRECTION = (LINE_PERIOD_TIME_US * 7) / 360;

// how many periods to read when calculating the power
static const uint32_t PERIODS_TO_READ = 20;

#ifndef RPI
#ifdef BCM2835
int bcm2835_init(void) { return 1; }
int bcm2835_close(void) { return 1; }
int bcm2835_spi_begin() { return 1; }
void bcm2835_spi_end() {}
void bcm2835_spi_setBitOrder(uint8_t order) {}
void bcm2835_spi_setDataMode(uint8_t mode) {}
void bcm2835_spi_setClockDivider(uint16_t divider) {}
void bcm2835_spi_setChipSelectPolarity(uint8_t cs, uint8_t active) {}
void bcm2835_spi_chipSelect(uint8_t cs) {}
void bcm2835_spi_transfernb(char *tbuf, char *rbuf, uint32_t len) {}
#endif
#endif

/**
 * RAII type class to set the thread priority to real time
 */
class Scheduler {
   public:
    Scheduler() : m_policy(sched_getscheduler(m_pid)) {
        sched_getparam(m_pid, &m_param);

        auto param = m_param;
        param.sched_priority = sched_get_priority_max(SCHED_FIFO);
        sched_setscheduler(m_pid, SCHED_FIFO, &param);
    }

    ~Scheduler() { sched_setscheduler(m_pid, m_policy, &m_param); }

   private:
    static const pid_t m_pid = 0;
    int m_policy;
    struct sched_param m_param;
};

Power::Power() : BackgroundTask(true) {
    auto &settings = Settings::getInstance();

    {
        auto hardware = settings.get("hardware");
        m_refVoltage = hardware.at("refVoltage");
        Log(INFO) << "refVoltage " << m_refVoltage;
        m_adcOffsetVoltage = hardware.at("adcOffsetVoltage");
        Log(INFO) << "adcOffsetVoltage " << m_adcOffsetVoltage;
    }

    auto voltageChannel = settings.get("voltageChannel");
    const std::string voltageChannelName = voltageChannel.at("name");
    const int voltageChipID = voltageChannel.at("chipID");
    const int voltageChannelID = voltageChannel.at("channelID");
    const int voltageChannelPhase = voltageChannel.at("phase");
    Log(INFO) << "Adding voltage channel " << voltageChannelName << " at " << voltageChipID << ":" << voltageChannelID
              << " phase " << voltageChannelPhase;
    m_voltageChannel.reset(new ChannelAD(voltageChannelName, voltageChipID, voltageChannelID, -1.f * ADC_OFFSET + CAL_OFFSET_VOLTAGE,
                        m_refVoltage * TRANSFORMER_LINE_VOLTAGE_RATIO * CAL_FACTOR_VOLTAGE));


    Log(INFO) << "Adding current channels...";
    auto currentChannels = settings.get("currentChannels");
    for (auto &channel : currentChannels) {
        const std::string channelName = channel.at("name");
        const int chipID = channel.at("chipID");
        const int channelID = channel.at("channelID");
        const float calibOffset = channel.at("calibOffset");
        const float resistance = channel.at("resistance");
        const int phase = channel.at("phase");
        Log(INFO) << " " << channelName << " at " << chipID << ":" << channelID << " calibOffset " << calibOffset
                  << " resistance " << resistance << " phase " << phase;

        // (U / R) * ratio
        const float calibFactor = (1.f / resistance) * 1860.f;
        const float calibTimeOffset = (LINE_PERIOD_TIME_US * (voltageChannelPhase - phase)) / 3;

        m_currentChannels.push_back(
            std::unique_ptr<ChannelAD>(new ChannelAD(channelName, chipID, channelID, -1.f * ADC_OFFSET + calibOffset,
                                                     m_refVoltage * calibFactor, calibTimeOffset)));

        // add a voltage channel for each current channel
        m_voltageChannels.push_back(std::unique_ptr<ChannelAD>(
            new ChannelAD(channelName + "_voltage", voltageChipID, voltageChannelID, -1.f * ADC_OFFSET + CAL_OFFSET_VOLTAGE,
                          m_refVoltage * TRANSFORMER_LINE_VOLTAGE_RATIO * CAL_FACTOR_VOLTAGE)));
    }

    // create the sum channels
    Log(INFO) << "Adding sum channels...";
    auto sumChannels = settings.get("sumChannels");
    for (auto &channel : sumChannels) {
        const std::string channelName = channel.at("name");
        Log(INFO) << " " << channelName << std::endl << " sources";
        m_channels.push_back(std::unique_ptr<ChannelSum>(new ChannelSum(channelName)));

        auto &c = m_channels.back();
        auto sources = channel.at("sources");
        for (auto &source : sources) {
            const std::string sourceName = source;
            Log(INFO) << "  " << sourceName;
            const auto it = std::find_if(
                m_currentChannels.begin(), m_currentChannels.end(),
                [&sourceName](const std::unique_ptr<ChannelAD> &channel) { return sourceName == channel->name(); });
            if (it == m_currentChannels.end()) {
                std::stringstream msg;
                msg << "Can't find channel " << source << " to be added to sum channel " << c->name();
                throw std::runtime_error(msg.str());
            }
            c->add(it->get());
        }
    }
    Log(INFO) << "..done";
}

Power::~Power() {}

static void read(uint32_t chipID, std::vector<Command> &cmds, std::vector<float> &values,
                 std::vector<timeValueUs> &times) {
#ifdef RPI
#ifdef BCM2835
    static const bcm2835SPIChipSelect cs[] = {BCM2835_SPI_CS0, BCM2835_SPI_CS1};

    // select the chip
    bcm2835_spi_chipSelect(cs[chipID]);
#endif  // BCM2835

    const size_t commandSize = sizeof(Command::m_sequence.m_data);
    unsigned char reply[commandSize];

    for (auto &&cmd : cmds) {
#ifdef BCM2835
        bcm2835_spi_transfernb(reinterpret_cast<char *>(cmd.m_sequence.m_data), reinterpret_cast<char *>(reply),
                               commandSize);
#elif defined(WIRINGPI)
        memcpy(reply, cmd.m_sequence.m_data, commandSize);
        wiringPiSPIDataRW(chipID, reply, commandSize);
#endif  // BCM2835
        times.push_back(time());

        uint32_t value = 0;
        for (size_t i = 0; i < commandSize; ++i) {
            value <<= 8;
            value += reply[i];
        }

        // mask out undefined bits
        value &= ADC_MASK;

        float normalized = (float)value / (float)ADC_MASK;
        values.push_back(normalized);
    }
#else  // RPI
    static timeValueUs startTime = 0;
    if (!startTime) startTime = time();
    const timeValueUs curTime = time() - startTime;
    const float wave = std::sin(std::fmod((float)curTime / 1000000.f / (1.f / LINE_FREQUENCY), 1.f) * 2.0f * PI);
    for (auto &&cmd : cmds) {
        // factor to calculate voltage from input signal
        static const float factor[2][8] = {
            {
                0.9f / CT_AMPERE_PER_VOLT,  // 146.37 W
                0.5f / CT_AMPERE_PER_VOLT,  // 81.32 W
                1.6f / CT_AMPERE_PER_VOLT,  // 260.22 W
                0.8f / CT_AMPERE_PER_VOLT,
                0.1f / CT_AMPERE_PER_VOLT,
                0.2f / CT_AMPERE_PER_VOLT,
                0.3f / CT_AMPERE_PER_VOLT,
                0.4f / CT_AMPERE_PER_VOLT,
            },
            {1.f / CT_AMPERE_PER_VOLT, 2.f / CT_AMPERE_PER_VOLT, 3.f / CT_AMPERE_PER_VOLT, 4.f / CT_AMPERE_PER_VOLT,
             5.f / CT_AMPERE_PER_VOLT, 6.f / CT_AMPERE_PER_VOLT,
             7.f / CT_AMPERE_PER_VOLT,  // 1138.44
             // voltage channel
             LINE_VOLTAGE_PEAK / ((120.f + 10.f) / 10.f) * TRANSFORMER_RATIO},
        };

        float value = wave * factor[chipID][cmd.m_sequence.m_bitfield.m_channel];
        // convert to -.5 ... .5
        value /= m_refVoltage / 2.f;
        // and to 0 ... 1
        value += 0.5f;
        values.push_back(value);
    }
#endif
}

void Power::update(std::unique_ptr<ChannelAD> &channel, std::unique_ptr<ChannelAD> &voltageChannel) {
    std::vector<std::vector<Command>> cmds;

    {
        const uint32_t chipID = channel->chipID();
        if (chipID >= cmds.size()) cmds.resize(chipID + 1);
        cmds[chipID].push_back(channel->command());
        channel->clearSamples();
    }

    const uint32_t chipID = m_voltageChannel->chipID();
    if (chipID >= cmds.size()) cmds.resize(chipID + 1);
    cmds[chipID].push_back(m_voltageChannel->command());
    m_voltageChannel->clearSamples();

    std::vector<std::vector<float>> values(cmds.size());
    std::vector<std::vector<timeValueUs>> times(cmds.size());
    std::vector<size_t> indices(cmds.size());

    // switch scheduler to high priority
    std::unique_ptr<Scheduler> scheduler(new Scheduler());

    // samling intervale (+2 periods for phase correction)
    const timeValueUs sampleTimeUs = LINE_PERIOD_TIME_US * (PERIODS_TO_READ + 2);
    auto startTimeUs = time();
    timeValueUs endTimeUs;

    // read and write to channels
    do {
        endTimeUs = time();

        for (auto &&value : values) value.clear();
        for (auto &&time : times) time.clear();

        uint32_t chipID = 0;
        for (auto &&chipCmd : cmds) {
            read(chipID, chipCmd, values[chipID], times[chipID]);
            ++chipID;
        }

        for (auto &&index : indices) index = 0;

        chipID = channel->chipID();
        channel->setSample(times[chipID][indices[chipID]], values[chipID][indices[chipID]]);
        indices[chipID]++;

        chipID = m_voltageChannel->chipID();
        m_voltageChannel->setSample(times[chipID][indices[chipID]], values[chipID][indices[chipID]]);
        indices[chipID]++;

    } while (endTimeUs - startTimeUs < sampleTimeUs);

    // back to standart priority
    scheduler.reset();

#if 0
    // find zero-crossing
    startTimeUs = 0;
    endTimeUs = 0;
    auto prevPositive = m_voltageChannel->value(0) >= 0.f;
    uint32_t halfPeriod = 0;
    uint32_t periods = 0;
    for (size_t index = 1; index < m_voltageChannel->sampleCount(); ++index)
    {
        const auto value = m_voltageChannel->value(index);
        const auto curPositive = value >= 0.f;
        if (curPositive != prevPositive)
        {
            const auto timeUs = m_voltageChannel->sampleTime(index);
            if (startTimeUs == 0)
            {
                startTimeUs = timeUs;
            }
            else
            {
                ++halfPeriod;
                if ((halfPeriod & 1) == 0)
                {
                    endTimeUs = timeUs;
                    ++periods;
                }
            }
            prevPositive = curPositive;
        }
    }
    //Log(DEBUG) << "Found " << periods << " periods, frequency " << periods / ((endTimeUs - startTimeUs) / 1000000.f) << " Hz";
#endif

    // calculate power
    voltageChannel->clearSamples();
    {
        // Log(DEBUG) << "Samples [" << channel->chipID() << ":" <<
        // channel->channelID() << "] " << channel->sampleCount();

        float p = 0.f;
        // float i = 0.f;
        // float u = 0.f;

        // set the sample interval to start one period after the first sample
        // and also leave one period space at the end. This is to have space for
        // the phase correction.
        const timeValueUs startSampleTimeUs = channel->sampleTime(0) + LINE_PERIOD_TIME_US;
        const timeValueUs endSampleTimeUs = startSampleTimeUs + PERIODS_TO_READ * LINE_PERIOD_TIME_US;
        for (size_t index = 0; index < channel->sampleCount() - 1; ++index) {
            const timeValueUs time = channel->sampleTime(index);
            // check if the time is in the interval, else continue with next
            // sample
            if (time < startSampleTimeUs) continue;
            // get the time of the next sample
            const timeValueUs nextTime = channel->sampleTime(index + 1);
            // if the next time is outside of the interval we are done
            if (nextTime > endSampleTimeUs) break;

            // get the current
            const float current = channel->value(index);
            // get the voltage at the time modified by the phase
            const auto voltageTime = time + channel->timeOffset() - CAL_PHASE_CORRECTION;
            // if the voltage time is outside of the interval we are done
            if ((voltageTime > endTimeUs) || (voltageTime < startTimeUs))
                Log(ERROR) << "Voltage time " << voltageTime << "out of range (" << startSampleTimeUs << ", "
                           << endSampleTimeUs << ")";
            const float voltage = m_voltageChannel->sampleAtTime(voltageTime);
            // write to voltage channel for this current
            voltageChannel->setSample(voltageTime, voltage);

            // i += current * current * (nextTime - time);
            // u += voltage * voltage * (nextTime - time);
            p += (voltage * current) * (nextTime - time);
        }

        const float invTimeRangeUs = 1.f / (float)(endSampleTimeUs - startSampleTimeUs);
        p *= invTimeRangeUs;

        // u *= invTimeRangeUs;
        // u = std::sqrt(u);
        // i *= invTimeRangeUs;
        // i = std::sqrt(i);
        // Log(DEBUG) << channel->name() << ": U " << u << " I " << i << " P "
        // << p;

        channel->set(p);
    }
}

void Power::preStart() {
#ifdef BCM2835
    if (!bcm2835_init()) throw std::runtime_error("Failed to init BCM 2835");

    if (!bcm2835_spi_begin()) throw std::runtime_error("bcm2835_spi_begin() failed");

    bcm2835_spi_setBitOrder(BCM2835_SPI_BIT_ORDER_MSBFIRST);
    bcm2835_spi_setDataMode(BCM2835_SPI_MODE0);                   // Data comes in on falling
                                                                  // edge
    bcm2835_spi_setClockDivider(BCM2835_SPI_CLOCK_DIVIDER_2048);  // 19.2MHz / 2048 = 9.375kHz
    bcm2835_spi_setChipSelectPolarity(BCM2835_SPI_CS0, LOW);
    bcm2835_spi_setChipSelectPolarity(BCM2835_SPI_CS1, LOW);
#elif defined(WIRINGPI)
    const int speed = 1 * 1000 * 1000;
    if (wiringPiSetup() == -1) throw std::runtime_error("Failed to setup WiringPI");
    int fd;
    fd = wiringPiSPISetup(0, speed);
    if (fd == -1) throw std::runtime_error("Failed to setup SPI channel 0");
    m_fds.push_back(fd);
    fd = wiringPiSPISetup(1, speed);
    if (fd == -1) throw std::runtime_error("Failed to setup SPI channel 1");
    m_fds.push_back(fd);
#endif
}

void Power::threadFunction() {
    std::vector<const ChannelAD *> channelsAD;
    auto itCurrent = m_currentChannels.begin();
    auto itVoltage = m_voltageChannels.begin();
    while (itCurrent != m_currentChannels.end()) {
        update(*itCurrent, *itVoltage);

        channelsAD.push_back(itCurrent->get());
        channelsAD.push_back(itVoltage->get());

        ++itCurrent;
        ++itVoltage;
    }

    std::vector<const Channel *> channels;
    for (auto &&channel : m_currentChannels) {
        channels.push_back(channel.get());
    }
    for (auto &&channel : m_channels) {
        channel->update();
        channels.push_back(channel.get());
    }

    post(channels);

    Server::getInstance().update(channelsAD);
}

void Power::postStop() {
#ifdef BCM2835
    bcm2835_spi_end();
    if (!bcm2835_close()) throw std::runtime_error("bcm2835_close() failed");
#elif defined(WIRINGPI)
    for (auto &&fd : m_fds) close(fd);
    m_fds.clear();
#endif
}
