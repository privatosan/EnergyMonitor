#include "Solar.h"
#include "Channel.h"
#include "Post.h"
#include "Options.h"

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

static const std::chrono::seconds UPDATE_PERIOD(10);

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
    CODE_KDY,   // Energy today [Wh]
    CODE_KLD,   // Energy last day [Wh]
    CODE_KMT,   // Energy this month [kWh]
    CODE_KLM,   // Energy last month [kWh]
    CODE_KYR,   // Energy this year [kWh]
    CODE_KLY,   // Energy last year [kWh]
    CODE_KT0,   // Energy total [kWh]
    CODE_LAN,   // Language
    CODE_UDC,   // DC voltage [mV]
    CODE_UL1,   // AC voltage [mV]
    CODE_IDC,   // DC current [mA]
    CODE_IL1,   // AC current [mA]
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

static const char *CodeString[] =
{
    "ADR",
    "TYP",
    "SWV",
    "DDY",
    "DMT",
    "DYR",
    "THR",
    "TMI",
    "E11",
    "E1D",
    "E1M",
    "E1h",
    "E1m",
    "E21",
    "E2D",
    "E2M",
    "E2h",
    "E2m",
    "E31",
    "E3D",
    "E3M",
    "E3h",
    "E3m",
    "KHR",
    "KDY",
    "KLD",
    "KMT",
    "KLM",
    "KYR",
    "KLY",
    "KT0",
    "LAN",
    "UDC",
    "UL1",
    "IDC",
    "IL1",
    "PAC",
    "PIN",
    "PRL",
    "CAC",
    "FRD",
    "SCD",
    "SE1",
    "SE1",
    "SPR",
    "TKK",
    "TNF",
    "SYS",
    "BDN",
    "EC00",
    "EC01",
    "EC02",
    "EC03",
    "EC04",
    "EC05",
    "EC06",
    "EC07",
    "EC08",
};

Solar::Solar()
    : m_stop(false)
{
    for (uint32_t address = START_ADDRESS; address <= END_ADDRESS; ++address)
    {
        std::ostringstream name;
        name << "solar_w" << address;
        m_channelsConverter.push_back(std::unique_ptr<ChannelConverter>(new ChannelConverter(name.str(), address)));
    }
    m_channelSolar.reset(new ChannelSum("solar", reinterpret_cast<const std::vector<std::unique_ptr<Channel>>&>(m_channelsConverter)));
}

Solar::~Solar()
{
}

static std::string buildMessage(std::vector<Code> codes, uint32_t dst)
{
    std::ostringstream data;

    bool first = true;
    for (auto&& code: codes)
    {
        if (!first)
            data << ";";
        first = false;
        data << CodeString[(uint32_t)code];
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

static std::vector<uint32_t> parseReply(uint32_t src, const std::string &reply, const std::vector<Code> codes)
{
    std::vector<uint32_t> result;

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
        std::string codeStr(CodeString[(uint32_t)code]);

        if (reply.compare(index, codeStr.length(), codeStr) != 0)
            throw std::runtime_error("Unexpected reply");
        index += codeStr.length();

        if (reply[index] != '=')
            throw std::runtime_error("Unexpected reply");
        index++;

        uint32_t value = 0;
        std::istringstream(reply.substr(index)) >> std::hex >> value;
        result.push_back(value);

        if (Options::getInstance().verbose())
            std::cout << codeStr << ": " << value << std::endl;
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
    std::vector<Code> codes;

    codes.push_back(Code::CODE_PAC);

    while (!m_stop)
    {
        for (auto &&channel : m_channelsConverter)
        {
            std::istringstream msg(buildMessage(codes, channel->address()));

            if (Options::getInstance().verbose())
                std::cout << "Solar: Send " << msg.str() << std::endl;

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

                if (Options::getInstance().verbose())
                    std::cout << "Solar: Reply " << reply.str() << std::endl;
                auto result = parseReply(channel->address(), reply.str(), codes);

                channel->set(result[0]);
            }
            catch (std::exception &er)
            {
                std::cerr << "Error: " << er.what() << std::endl;
            }
            if (curl)
                curl_easy_cleanup(curl);
        }

        std::vector<const Channel*> channels;
        for (auto &&channel : m_channelsConverter)
            channels.push_back(channel.get());

        m_channelSolar->update();
        channels.push_back(m_channelSolar.get());

        post(channels);

        std::unique_lock<std::mutex> lock(cv_mutex);
        m_conditionVariable.wait_for(lock, UPDATE_PERIOD);
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
