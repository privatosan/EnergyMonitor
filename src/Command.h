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
            unsigned char m_start : 1;
            unsigned char m_zeroes : 7;
            unsigned char m_ignored : 4;
            unsigned char m_channel : 3;
            unsigned char m_single : 1;
        } m_bitfield;
        unsigned char m_data[3];
    } m_sequence;
};

#endif // COMMAND_H
