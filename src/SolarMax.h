#ifndef SOLARMAX_H
#define SOLARMAX_H

#include <string>
#include <vector>

namespace SolarMax
{

class Value
{
public:
    Value(const std::string &code);
    ~Value();

    const std::string &code() const
    {
        return m_code;
    }

    float value() const
    {
        return m_fvalue.m_value;
    }

    float max() const
    {
        return m_stat.m_max;
    }

    void parse(const std::string &value);

private:
    std::string m_code;
    union
    {
        struct
        {
            float m_factor;
            float m_value;
        } m_fvalue;
        struct
        {
            uint32_t m_year;
            uint32_t m_month;
            uint32_t m_day;
            float m_power;
            float m_max;
            float m_hours;
        } m_stat;
    };
};

bool askConverter(uint32_t address, std::vector<Value*> &values);

} // namespace SolarMax

#endif // SOLARMAX_H
