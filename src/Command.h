#ifndef COMMAND_H
#define COMMAND_H

#include <memory.h>

class Command
{
public:
    Command(unsigned int channel)
    {
        memset(m_sequence.m_data, 0, sizeof(m_sequence.m_data));
        m_sequence.m_bitfield.m_start = 1;
        m_sequence.m_bitfield.m_single = 1;
        m_sequence.m_bitfield.m_channel = channel;
    }

    union
    {
        struct
        {
            char m_start : 1;
            char m_zeroes : 7;
            char m_ignored : 4;
            char m_channel : 3;
            char m_single : 1;
        } m_bitfield;
        char m_data[3];
    } m_sequence;
};

#endif // COMMAND_H
