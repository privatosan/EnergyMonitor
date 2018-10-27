#include "Solar.h"
#include "Channel.h"
#include "Post.h"
#include "Options.h"
#include "Log.h"

#include <curlpp/cURLpp.hpp>
#include <curlpp/Easy.hpp>
#include <curlpp/Options.hpp>

#include <iostream>
#include <thread>
#include <vector>
#include <sstream>
#include <iomanip>
#include <stdexcept>

static const uint32_t START_ADDRESS = 1;
static const uint32_t END_ADDRESS = 3;

Solar::Solar()
    : m_stop(false)
{
    m_channelSolar.push_back(std::unique_ptr<ChannelSum>(new ChannelSum("solar")));
    m_channelSolar.push_back(std::unique_ptr<ChannelSum>(new ChannelSum("solar_kwh")));

    for (uint32_t address = START_ADDRESS; address <= END_ADDRESS; ++address)
    {
        std::vector<std::unique_ptr<ChannelConverter>> channels;
        std::unique_ptr<ChannelConverter> channel;

        std::ostringstream name;
        name << "solar_w" << address;
        channel.reset(new ChannelConverter(name.str(), address, new SolarMax::Value("PAC")));
        m_channelSolar[0]->add(channel.get());
        channels.push_back(std::move(channel));

        name.str("");
        name << "solar_kwh" << address;
        channel.reset(new ChannelConverter(name.str(), address, new SolarMax::Value("KDY")));
        m_channelSolar[1]->add(channel.get());
        channels.push_back(std::move(channel));

        m_channelsConverter.push_back(std::move(channels));
    }
}

Solar::~Solar()
{
}

void Solar::getStat(const std::string file, const std::string start, std::vector<Solar::Stat> &stats) const
{
    std::ostringstream reply;

    try
    {
        curlpp::Cleanup cleaner;
        curlpp::Easy request;

        request.setOpt(new curlpp::Options::Url(std::string("ftp://privatosan.bplaced.net/PV-Anlage/") + file));
        request.setOpt(new curlpp::Options::UserPwd("privatosan:vallejo71"));

        reply << request;
    }
    catch (std::exception &er)
    {
        Log(ERROR) << er.what();
    }

    std::istringstream input(reply.str());
    std::string line;
    while (std::getline(input, line))
    {
        size_t index = 0;
        size_t newIndex = line.find_first_of(start, index);
        if (newIndex == std::string::npos)
        {
            Log(ERROR) << "Invalid string";
            continue;
        }
        index = newIndex + start.length();

        Stat stat;

        std::istringstream(line.substr(index, 2)) >> stat.m_day;
        index += 2;
        if (line[index] != '.')
        {
            Log(ERROR) << "Invalid string";
            continue;
        }
        ++index;

        std::istringstream(line.substr(index, 2)) >> stat.m_month;
        index += 2;
        if (line[index] != '.')
        {
            Log(ERROR) << "Invalid string";
            continue;
        }
        ++index;

        std::istringstream(line.substr(index, 2)) >> stat.m_year;
        index += 2;
        if (line[index] != '|')
        {
            Log(ERROR) << "Invalid string";
            continue;
        }
        ++index;

        std::istringstream values(line.substr(index));
        std::string valueStr;
        while (std::getline(values, valueStr, '|'))
        {
            uint32_t value;
            std::istringstream(valueStr) >> value;
            stat.m_values.push_back(value);
        }

        stats.push_back(stat);
    }
}

void Solar::readStat()
{
    getStat("days_hist.js", "da[dx++]=\"", m_days);
    getStat("months.js", "mo[mx++]=\"", m_months);
    getStat("years.js", "ye[yx++]=\"", m_years);
}

void Solar::putStat(const std::string file, const std::string start, const std::vector<Solar::Stat> &stats) const
{
    std::ostringstream output;

    for (auto&& stat: stats)
    {
        output << start <<
            std::setfill('0') << std::setw(2) << stat.m_day << '.' <<
            std::setfill('0') << std::setw(2) << stat.m_month << '.' <<
            std::setfill('0') << std::setw(2) << stat.m_year << '|';

        bool first = true;
        for (auto&& value: stat.m_values)
        {
            output << (first ? "" : "|") << value;
            first = false;
        }
        output << '"' << std::endl;
    }

    try
    {
        curlpp::Cleanup cleaner;
        curlpp::Easy request;

        request.setOpt(new curlpp::Options::Url(std::string("ftp://privatosan.bplaced.net/PV-Anlage/new") + file));
        request.setOpt(new curlpp::Options::UserPwd("privatosan:vallejo71"));

        std::istringstream stream(output.str());
        request.setOpt(new curlpp::Options::ReadStream(&stream));
        request.setOpt(new curlpp::Options::InfileSize(stream.str().size()));

        request.setOpt(new curlpp::Options::Upload(true));

        request.perform();
    }
    catch (std::exception &er)
    {
        Log(ERROR) << er.what();
    }
}

void Solar::writeStat()
{
    putStat("days_hist.js", "da[dx++]=\"", m_days);
    putStat("months.js", "mo[mx++]=\"", m_months);
    putStat("years.js", "ye[yx++]=\"", m_years);
}

/*void Solar::readStatistics()
{
    time_t curTime = time(nullptr);
    if (curTime == (time_t)-1)
    {
        Log(ERROR) << "Invalid time";
        return;
    }
    tm localTime;
    if (!localtime_r(&curTime, &localTime))
    {
        Log(ERROR) << "localtime() failed";
        return;
    }

    if ((localTime.tm_mday == m_lastStatusUpdate.tm_mday) &&
        (localTime.tm_year == m_lastStatusUpdate.tm_year))
    {
        return;
    }

    for (uint32_t address = START_ADDRESS; address <= END_ADDRESS; ++address)
    {
        for (uint32_t day = 0; day <= localTime.tm_mday; ++day)
        {
            std::vector<SolarMax::Value*> values;

            std::ostringstream code;
            code << "DD" << std::setfill('0') << std::setw(2) << day;
            std::unique_ptr<SolarMax::Value> value(new SolarMax::Value(code.str()));

            values.push_back(value.get());

            if (!askConverter(address, values))
            {
                return;
            }
            std::cout << (*values.begin())->value() << std::endl;
        }
    }

    m_lastStatusUpdate = localTime;
}*/


void Solar::threadFunction()
{
    while (!m_stop)
    {
        uint32_t activeConverter = 0;
        for (auto &&channels : m_channelsConverter)
        {
            std::vector<SolarMax::Value*> values;
            uint32_t address = 0;

            for (auto &&channel : channels)
            {
                if (address == 0)
                    address = channel->address();
                else if(address != channel->address())
                    throw std::runtime_error("Channels of one group need to have the same address");

                values.push_back(channel->value());
            }

            if (SolarMax::askConverter(address, values))
            {
                ++activeConverter;
                for (auto &&channel : channels)
                    channel->set(channel->value()->value());
            }
        }

        if (activeConverter != 0)
        {
            std::vector<const Channel*> channels;
            for (auto &&channelsConverter : m_channelsConverter)
            {
                for (auto &&channel : channelsConverter)
                    channels.push_back(channel.get());
            }

            for (auto &&channelSolar : m_channelSolar)
            {
                channelSolar->update();
                channels.push_back(channelSolar.get());
            }

            post(channels);
        }

        std::unique_lock<std::mutex> lock(cv_mutex);
        m_conditionVariable.wait_for(lock, Options::getInstance().updatePeriod());
    }
}

void Solar::start()
{
    if (m_thread)
        throw std::runtime_error("Thread already running");

    readStat();
    writeStat();

    m_thread.reset(new std::thread(&Solar::threadFunction, this));
}

void Solar::stop()
{
    if (!m_thread)
        throw std::runtime_error("No thread running");

    {
        std::lock_guard<std::mutex> lock(cv_mutex);
        m_stop = true;
    }
    m_conditionVariable.notify_all();

    m_thread->join();
    m_thread.reset();
}

enum class Code
{
    CODE_ADR,   // Address
    CODE_TYP,   // Type
    CODE_SWV,   // Software version
    CODE_DDY,   // Date day
    CODE_DMT,   // Date month
    CODE_DYR,   // Date year
    CODE_THR,   // Time hours
    CODE_TMI,   // Time minutes
    CODE_E11,   // Error 1, number
    CODE_E1D,   // Error 1, day
    CODE_E1M,   // Error 1, month
    CODE_E1h,   // Error 1, hour
    CODE_E1m,   // Error 1, minute
    CODE_E21,   // Error 2, number
    CODE_E2D,   // Error 2, day
    CODE_E2M,   // Error 2, month
    CODE_E2h,   // Error 2, hour
    CODE_E2m,   // Error 2, minute
    CODE_E31,   // Error 3, number
    CODE_E3D,   // Error 3, day
    CODE_E3M,   // Error 3, month
    CODE_E3h,   // Error 3, hour
    CODE_E3m,   // Error 3, minute
    CODE_KHR,   // Operating hours
    CODE_KDY,   // Energy today [kWh]
    CODE_KLD,   // Energy last day [kWh]
    CODE_KMT,   // Energy this month [kWh]
    CODE_KLM,   // Energy last month [kWh]
    CODE_KYR,   // Energy this year [kWh]
    CODE_KLY,   // Energy last year [kWh]
    CODE_KT0,   // Energy total [kWh]
    CODE_LAN,   // Language
    CODE_UDC,   // DC voltage [V]
    CODE_UL1,   // AC voltage [V]
    CODE_IDC,   // DC current [A]
    CODE_IL1,   // AC current [A]
    CODE_PAC,   // AC power [W]
    CODE_PIN,   // Power installed [W]
    CODE_PRL,   // AC power [%]
    CODE_CAC,   // Start ups
    CODE_FRD,   // ???
    CODE_SCD,   // ???
    CODE_SE1,   // ???
    CODE_SE2,   // ???
    CODE_SPR,   // ???
    CODE_TKK,   // Temperature Heat Sink
    CODE_TNF,   // Net frequency (Hz)
    CODE_SYS,   // Operation State
    CODE_BDN,   // Build number
    CODE_EC00,  // Error-Code(?) 00
    CODE_EC01,  // Error-Code(?) 01
    CODE_EC02,  // Error-Code(?) 02
    CODE_EC03,  // Error-Code(?) 03
    CODE_EC04,  // Error-Code(?) 04
    CODE_EC05,  // Error-Code(?) 05
    CODE_EC06,  // Error-Code(?) 06
    CODE_EC07,  // Error-Code(?) 07
    CODE_EC08,  // Error-Code(?) 08
    CODE_DD00,  // Statistic Day 0 YYYMMDD,Yield kWh*10,Peak W*2,Hours*10
    CODE_DD01,  // Statistic Day 1 YYYMMDD,Yield kWh*10,Peak W*2,Hours*10
    CODE_DD02,  // Statistic Day 2 YYYMMDD,Yield kWh*10,Peak W*2,Hours*10
    CODE_DD03,  // Statistic Day 3 YYYMMDD,Yield kWh*10,Peak W*2,Hours*10
    CODE_DD04,  // Statistic Day 4 YYYMMDD,Yield kWh*10,Peak W*2,Hours*10
    CODE_DD05,  // Statistic Day 5 YYYMMDD,Yield kWh*10,Peak W*2,Hours*10
    CODE_DD06,  // Statistic Day 6 YYYMMDD,Yield kWh*10,Peak W*2,Hours*10
    CODE_DD07,  // Statistic Day 7 YYYMMDD,Yield kWh*10,Peak W*2,Hours*10
    CODE_DD08,  // Statistic Day 8 YYYMMDD,Yield kWh*10,Peak W*2,Hours*10
    CODE_DD09,  // Statistic Day 9 YYYMMDD,Yield kWh*10,Peak W*2,Hours*10
    CODE_DD10,  // Statistic Day 10 YYYMMDD,Yield kWh*10,Peak W*2,Hours*10
    CODE_DD11,  // Statistic Day 11 YYYMMDD,Yield kWh*10,Peak W*2,Hours*10
    CODE_DD12,  // Statistic Day 12 YYYMMDD,Yield kWh*10,Peak W*2,Hours*10
    CODE_DD13,  // Statistic Day 13 YYYMMDD,Yield kWh*10,Peak W*2,Hours*10
    CODE_DD14,  // Statistic Day 14 YYYMMDD,Yield kWh*10,Peak W*2,Hours*10
    CODE_DD15,  // Statistic Day 15 YYYMMDD,Yield kWh*10,Peak W*2,Hours*10
    CODE_DD16,  // Statistic Day 16 YYYMMDD,Yield kWh*10,Peak W*2,Hours*10
    CODE_DD17,  // Statistic Day 17 YYYMMDD,Yield kWh*10,Peak W*2,Hours*10
    CODE_DD18,  // Statistic Day 18 YYYMMDD,Yield kWh*10,Peak W*2,Hours*10
    CODE_DD19,  // Statistic Day 19 YYYMMDD,Yield kWh*10,Peak W*2,Hours*10
    CODE_DD20,  // Statistic Day 20 YYYMMDD,Yield kWh*10,Peak W*2,Hours*10
    CODE_DD21,  // Statistic Day 21 YYYMMDD,Yield kWh*10,Peak W*2,Hours*10
    CODE_DD22,  // Statistic Day 22 YYYMMDD,Yield kWh*10,Peak W*2,Hours*10
    CODE_DD23,  // Statistic Day 23 YYYMMDD,Yield kWh*10,Peak W*2,Hours*10
    CODE_DD24,  // Statistic Day 24 YYYMMDD,Yield kWh*10,Peak W*2,Hours*10
    CODE_DD25,  // Statistic Day 25 YYYMMDD,Yield kWh*10,Peak W*2,Hours*10
    CODE_DD26,  // Statistic Day 26 YYYMMDD,Yield kWh*10,Peak W*2,Hours*10
    CODE_DD27,  // Statistic Day 27 YYYMMDD,Yield kWh*10,Peak W*2,Hours*10
    CODE_DD28,  // Statistic Day 28 YYYMMDD,Yield kWh*10,Peak W*2,Hours*10
    CODE_DD29,  // Statistic Day 29 YYYMMDD,Yield kWh*10,Peak W*2,Hours*10
    CODE_DD30,  // Statistic Day 30 YYYMMDD,Yield kWh*10,Peak W*2,Hours*10
    CODE_DM00,  // Statistic Month 0 YYYMM00,Yield kWh*10,Peak W*2,Hours*10
    CODE_DM01,  // Statistic Month 1 YYYMM00,Yield kWh*10,Peak W*2,Hours*10
    CODE_DM02,  // Statistic Month 2 YYYMM00,Yield kWh*10,Peak W*2,Hours*10
    CODE_DM03,  // Statistic Month 3 YYYMM00,Yield kWh*10,Peak W*2,Hours*10
    CODE_DM04,  // Statistic Month 4 YYYMM00,Yield kWh*10,Peak W*2,Hours*10
    CODE_DM05,  // Statistic Month 5 YYYMM00,Yield kWh*10,Peak W*2,Hours*10
    CODE_DM06,  // Statistic Month 6 YYYMM00,Yield kWh*10,Peak W*2,Hours*10
    CODE_DM07,  // Statistic Month 7 YYYMM00,Yield kWh*10,Peak W*2,Hours*10
    CODE_DM08,  // Statistic Month 8 YYYMM00,Yield kWh*10,Peak W*2,Hours*10
    CODE_DM09,  // Statistic Month 9 YYYMM00,Yield kWh*10,Peak W*2,Hours*10
    CODE_DM10,  // Statistic Month 10 YYYMM00,Yield kWh*10,Peak W*2,Hours*10
    CODE_DM11,  // Statistic Month 11 YYYMM00,Yield kWh*10,Peak W*2,Hours*10
    CODE_DY00,  // Statistic Year 0 YYY0000,Yield kWh*10,Peak W*2,Hours*10
    CODE_DY01,  // Statistic Year 1 YYY0000,Yield kWh*10,Peak W*2,Hours*10
    CODE_DY02,  // Statistic Year 2 YYY0000,Yield kWh*10,Peak W*2,Hours*10
    CODE_DY03,  // Statistic Year 3 YYY0000,Yield kWh*10,Peak W*2,Hours*10
    CODE_DY04,  // Statistic Year 4 YYY0000,Yield kWh*10,Peak W*2,Hours*10
    CODE_DY05,  // Statistic Year 5 YYY0000,Yield kWh*10,Peak W*2,Hours*10
    CODE_DY06,  // Statistic Year 6 YYY0000,Yield kWh*10,Peak W*2,Hours*10
    CODE_DY07,  // Statistic Year 7 YYY0000,Yield kWh*10,Peak W*2,Hours*10
    CODE_DY08,  // Statistic Year 8 YYY0000,Yield kWh*10,Peak W*2,Hours*10
    CODE_DY09,  // Statistic Year 9 YYY0000,Yield kWh*10,Peak W*2,Hours*10
};

