#ifndef SOLAR_H
#define SOLAR_H

#include "Channel.h"

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

    void threadFunction();
};

#endif // SOLAR_H
