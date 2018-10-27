#include "SolarMax.h"
#include "Log.h"

#include <curl/curl.h>

// had been renamed
#ifndef CURLINFO_ACTIVESOCKET
#define CURLINFO_ACTIVESOCKET CURLINFO_LASTSOCKET
#endif

#include <stdexcept>
#include <sstream>
#include <iomanip>

static const std::string IP_ADDR("192.168.178.123");
static const uint32_t PORT = 12345;

namespace SolarMax
{

Value::Value(const std::string &code)
    : m_code(code)
{
    if ((m_code == "PAC") || (m_code == "PIN"))
    {
        m_fvalue.m_factor = 0.5f;
    }
    else if ((m_code == "UDC") || (m_code == "UL1") || (m_code == "KDY") ||
        (m_code == "KLD"))
    {
        m_fvalue.m_factor = 0.1f;
    }
    else if ((m_code == "IDC") || (m_code == "IL1"))
    {
        m_fvalue.m_factor = 0.01f;
    }
    else if (m_code == "TNF")
    {
        m_fvalue.m_factor = 10.f;
    }
    else if (m_code.compare(0, 2, "DD") == 0)
    {
    }
    else
        throw std::runtime_error("Unhandled code");

    // the converter value is 3% lower than the reference value from the
    // electric meter
    if ((m_code == "PAC") || (m_code == "KDY"))
    {
        m_fvalue.m_factor *= 1.03f;
    }
}

Value::~Value()
{

}

void Value::parse(const std::string &string)
{
    if (m_code.compare(0, 2, "DD") == 0)
    {
        // YYYMMDD,kWh,max,h
        uint32_t year, month, day, power, max, hours;

        size_t index = 0;
        std::istringstream(string.substr(index, 3)) >> std::hex >> year;
        index += 3;
        std::istringstream(string.substr(index, 2)) >> std::hex >> month;
        index += 2;
        std::istringstream(string.substr(index, 2)) >> std::hex >> day;
        index += 2;
        if (string[index] != ',')
            return;
        ++index;

        std::istringstream(string.substr(index)) >> std::hex >> power;
        size_t newIndex = string.find_first_of(',', index);
        if (newIndex == std::string::npos)
            return;
        index = newIndex + 1;

        std::istringstream(string.substr(index)) >> std::hex >> max;
        newIndex = string.find_first_of(',', index);
        if (newIndex == std::string::npos)
            return;
        index = newIndex + 1;

        std::istringstream(string.substr(index)) >> std::hex >> hours;
        newIndex = string.find_first_of(',', index);
        if (newIndex == std::string::npos)
            return;
        index = newIndex + 1;

        m_stat.m_year = year;
        m_stat.m_month = month;
        m_stat.m_day = day;
        m_stat.m_power = power;
        m_stat.m_max = max;
        m_stat.m_hours = hours;
    }
    else
    {
        uint32_t ivalue = 0;

        std::istringstream(string) >> std::hex >> ivalue;
        m_fvalue.m_value = (float)ivalue * m_fvalue.m_factor;
    }
}

static std::string buildMessage(std::vector<Value*> values, uint32_t dst)
{
    std::ostringstream data;

    bool first = true;
    for (auto&& value: values)
    {
        if (!first)
            data << ";";
        first = false;
        data << value->code();
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

static void parseReply(uint32_t src, const std::string &reply, const std::vector<Value*> values)
{
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

    for (auto&& value: values)
    {
        // check for code
        const std::string codeStr = value->code();
        if (reply.compare(index, codeStr.length(), codeStr) != 0)
            throw std::runtime_error("Unexpected reply");
        index += codeStr.length();

        // skip the '='
        if (reply[index] != '=')
            throw std::runtime_error("Unexpected reply");
        index++;

        // get the value
        value->parse(reply.substr(index));

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

bool askConverter(uint32_t address, std::vector<Value*> &values)
{
    bool success = true;
    std::istringstream msg(buildMessage(values, address));

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

        Log(DEBUG) << "Solar: Reply " << reply.str();

        parseReply(address, reply.str(), values);
    }
    catch (std::exception &er)
    {
        Log(ERROR) << er.what();
        success = false;
    }
    if (curl)
        curl_easy_cleanup(curl);

    return success;
}

} // namespace SolarMax
