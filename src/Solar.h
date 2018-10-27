#ifndef SOLAR_H
#define SOLAR_H

#include "Channel.h"

#include <time.h>

#include <memory>
#include <thread>
#include <condition_variable>
#include <vector>

class ChannelConverter : public Channel
{
public:
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

    ChannelConverter(std::string name, uint32_t address, Code code)
        : Channel(name)
        , m_address(address)
        , m_code(code)
    {
    }

    uint32_t address() const
    {
        return m_address;
    }

    Code code() const
    {
        return m_code;
    }

 private:
    uint32_t m_address;
    Code m_code;
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

    tm m_lastStatusUpdate;

    void threadFunction();
    void readStatistics();
};

#endif // SOLAR_H
