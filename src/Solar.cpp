#include "Solar.h"
#include "Channel.h"
#include "Post.h"
#include "Options.h"
#include "Log.h"

#include <curl/curl.h>

// had been renamed
#ifndef CURLINFO_ACTIVESOCKET
#define CURLINFO_ACTIVESOCKET CURLINFO_LASTSOCKET
#endif

#include <iostream>
#include <thread>
#include <vector>
#include <sstream>
#include <iomanip>
#include <stdexcept>

static const std::string IP_ADDR("192.168.178.123");
static const uint32_t PORT = 12345;
static const uint32_t START_ADDRESS = 1;
static const uint32_t END_ADDRESS = 3;

struct CodeProperties
{
    const char *name;
    float factor;
};

static CodeProperties Codes[] =
{
    { "ADR", 1.f },
    { "TYP", 1.f },
    { "SWV", 1.f },
    { "DDY", 1.f },
    { "DMT", 1.f },
    { "DYR", 1.f },
    { "THR", 1.f },
    { "TMI", 1.f },
    { "E11", 1.f },
    { "E1D", 1.f },
    { "E1M", 1.f },
    { "E1h", 1.f },
    { "E1m", 1.f },
    { "E21", 1.f },
    { "E2D", 1.f },
    { "E2M", 1.f },
    { "E2h", 1.f },
    { "E2m", 1.f },
    { "E31", 1.f },
    { "E3D", 1.f },
    { "E3M", 1.f },
    { "E3h", 1.f },
    { "E3m", 1.f },
    { "KHR", 1.f },
    { "KDY", .1f },
    { "KLD", .1f },
    { "KMT", 1.f },
    { "KLM", 1.f },
    { "KYR", 1.f },
    { "KLY", 1.f },
    { "KT0", 1.f },
    { "LAN", 1.f },
    { "UDC", .1f },
    { "UL1", .1f },
    { "IDC", .01f },
    { "IL1", .01f },
    { "PAC", .5f },
    { "PIN", .5f },
    { "PRL", 1.f },
    { "CAC", 1.f },
    { "FRD", 1.f },
    { "SCD", 1.f },
    { "SE1", 1.f },
    { "SE1", 1.f },
    { "SPR", 1.f },
    { "TKK", 1.f },
    { "TNF", 10.f },
    { "SYS", 1.f },
    { "BDN", 1.f },
    { "EC00", 1.f },
    { "EC01", 1.f },
    { "EC02", 1.f },
    { "EC03", 1.f },
    { "EC04", 1.f },
    { "EC05", 1.f },
    { "EC06", 1.f },
    { "EC07", 1.f },
    { "EC08", 1.f },
};

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
        channel.reset(new ChannelConverter(name.str(), address, ChannelConverter::Code::CODE_PAC));
        m_channelSolar[0]->add(channel.get());
        channels.push_back(std::move(channel));

        name.str("");
        name << "solar_kwh" << address;
        channel.reset(new ChannelConverter(name.str(), address, ChannelConverter::Code::CODE_KDY));
        m_channelSolar[1]->add(channel.get());
        channels.push_back(std::move(channel));

        m_channelsConverter.push_back(std::move(channels));
    }
}

Solar::~Solar()
{
}

static std::string buildMessage(std::vector<ChannelConverter::Code> codes, uint32_t dst)
{
    std::ostringstream data;

    bool first = true;
    for (auto&& code: codes)
    {
        if (!first)
            data << ";";
        first = false;
        data << Codes[(uint32_t)code].name;
    }

    const uint32_t src = 0xFB;
    const size_t len =
          1 // '{'
        + 2 // 'FB' - source address (hex)
        + 1 // ';'
        + 2 // 'XX' - destination address (hex)
        + 1 // ';'
        + 2 // 'XX' - message length (complete message) (hex)
        + 4 // '|64:'
        + data.str().length() // data, codes separated by ';'
        + 1 // '|'
        + 4 // 'XXXX' - checksum (from source address to last char before checksum) (hex)
        + 1;// '}'

    std::ostringstream checksumMsg;
    checksumMsg <<
        std::setfill('0') << std::hex << std::uppercase << std::setw(2) << src << ";" <<
        std::setfill('0') << std::hex << std::uppercase << std::setw(2) << dst << ";" <<
        std::setfill('0') << std::hex << std::uppercase << std::setw(2) << len << "|64:" <<
        data.str() << "|";

    uint16_t checksum = 0;
    for (auto&& c: checksumMsg.str())
    {
        checksum += c;
    }

    std::ostringstream msg;
    msg <<
        "{" << checksumMsg.str() <<
        std::setfill('0') << std::hex << std::uppercase << std::setw(4) << checksum << "}";

    return msg.str();
}

static std::vector<float> parseReply(uint32_t src, const std::string &reply, const std::vector<ChannelConverter::Code> codes)
{
    std::vector<float> result;

    const uint32_t dst = 0xFB;
    std::ostringstream expected;
    expected <<
        "{" <<
        std::setfill('0') << std::hex << std::uppercase << std::setw(2) << src << ";" <<
        std::setfill('0') << std::hex << std::uppercase << std::setw(2) << dst << ";";

    size_t index = 0;
    if (reply.compare(index, expected.str().length(), expected.str()) != 0)
        throw std::runtime_error("Unexpected reply");
    index += expected.str().length();

    // length
    index += 2;

    // '|64:'
    std::string delimiter("|64:");
    if (reply.compare(index, delimiter.length(), delimiter) != 0)
        throw std::runtime_error("Unexpected reply");
    index += delimiter.length();

    for (auto&& code: codes)
    {
        // check for code
        std::string codeStr(Codes[(uint32_t)code].name);
        if (reply.compare(index, codeStr.length(), codeStr) != 0)
            throw std::runtime_error("Unexpected reply");
        index += codeStr.length();

        // skip the '='
        if (reply[index] != '=')
            throw std::runtime_error("Unexpected reply");
        index++;

        // get the value
        uint32_t value = 0;
        std::istringstream(reply.substr(index)) >> std::hex >> value;
        float fvalue = (float)value * Codes[(uint32_t)code].factor;
        result.push_back(fvalue);

        Log(DEBUG) << codeStr << ": " << fvalue;

        // skip ';'
        size_t newIndex = reply.find_first_of(';', index);
        if (newIndex == std::string::npos)
        {
            newIndex = reply.find_first_of('|', index);
            if (newIndex == std::string::npos)
                throw std::runtime_error("Unexpected reply, expected ';' or '|'");
        }
        index = newIndex + 1;
    }

    return result;
}

static int waitOnSocket(curl_socket_t sockfd, bool forRecv, long timeoutMs)
{
    struct timeval tv;
    fd_set infd, outfd, errfd;
    int res;

    tv.tv_sec = timeoutMs / 1000;
    tv.tv_usec= (timeoutMs % 1000) * 1000;

    FD_ZERO(&infd);
    FD_ZERO(&outfd);
    FD_ZERO(&errfd);

    FD_SET(sockfd, &errfd); /* always check for error */

    if(forRecv)
    {
        FD_SET(sockfd, &infd);
    }
    else
    {
        FD_SET(sockfd, &outfd);
    }

    /* select() returns the number of signalled sockets or -1 */
    res = select(sockfd + 1, &infd, &outfd, &errfd, &tv);
    return res;
}



void Solar::threadFunction()
{
    while (!m_stop)
    {
        uint32_t activeConverter = 0;
        for (auto &&channels : m_channelsConverter)
        {
            std::vector<ChannelConverter::Code> codes;
            uint32_t address = 0;

            for (auto &&channel : channels)
            {
                if (address == 0)
                    address = channel->address();
                else if(address != channel->address())
                    throw std::runtime_error("Channels of one group need to have the same address");

                codes.push_back(channel->code());
            }

            std::istringstream msg(buildMessage(codes, address));

            Log(DEBUG) << "Solar: Send " << msg.str();

            CURL *curl = nullptr;
            try
            {
                std::ostringstream reply;
                CURLcode res;

                curl = curl_easy_init();
                if (!curl)
                    throw std::runtime_error("Could not init curl");

                curl_easy_setopt(curl, CURLOPT_URL, IP_ADDR.c_str());
                curl_easy_setopt(curl, CURLOPT_PORT, PORT);
                curl_easy_setopt(curl, CURLOPT_CONNECT_ONLY, 1L);

                res = curl_easy_perform(curl);
                if (res != CURLE_OK)
                    throw std::runtime_error(curl_easy_strerror(res));

                curl_socket_t sockfd;
                res = curl_easy_getinfo(curl, CURLINFO_ACTIVESOCKET, &sockfd);
                if (res != CURLE_OK)
                    throw std::runtime_error(curl_easy_strerror(res));

                size_t nsent = 0;
                res = curl_easy_send(curl, msg.str().c_str(), msg.str().size(), &nsent);
                if (res != CURLE_OK)
                    throw std::runtime_error(curl_easy_strerror(res));

                char buf[512];
                size_t nread;
                do
                {
                    nread = 0;
                    res = curl_easy_recv(curl, buf, sizeof(buf), &nread);
                    if (res == CURLE_AGAIN)
                    {
                        if (!waitOnSocket(sockfd, true, 1000L))
                            throw std::runtime_error("Timeout");
                    }
                    if (nread != 0)
                    {
                        buf[nread] = 0;
                        reply << buf;
                    }
                } while (res == CURLE_AGAIN);

                if (res != CURLE_OK)
                    throw std::runtime_error(curl_easy_strerror(res));

                ++activeConverter;

                Log(DEBUG) << "Solar: Reply " << reply.str();

                auto result = parseReply(address, reply.str(), codes);

                if (result.size() != channels.size())
                    throw std::runtime_error("Expected result count equal to channels count");

                auto it = result.cbegin();
                for (auto &&channel : channels)
                {
                    channel->set(*it);
                    ++it;
                }
            }
            catch (std::exception &er)
            {
                Log(ERROR) << er.what();
            }
            if (curl)
                curl_easy_cleanup(curl);
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
