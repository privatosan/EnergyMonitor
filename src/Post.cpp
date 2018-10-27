#include "Post.h"
#include "Options.h"
#include "Log.h"

#include <curlpp/cURLpp.hpp>
#include <curlpp/Easy.hpp>
#include <curlpp/Options.hpp>

#include <sstream>

static const std::string IP_ADDR("127.0.0.1");

void post(const std::vector<const Channel*> &channels)
{
    std::ostringstream url;

    url << "http://" << IP_ADDR << "/emoncms/input/post.json?node=1&json={";

    bool addComma = false;
    for (auto&& channel : channels)
    {
        if (addComma)
            url << ",";
        url << channel->name() << ":" << channel->value();
        addComma = true;
    }
    url << "}&apikey=ab7fc3c6f67f0b0369cc1aca4afd9e4c";
    try
    {
        curlpp::Cleanup cleaner;

        Log(DEBUG) << "Post msg: " << url.str();

        std::ostringstream reply;
        reply << curlpp::options::Url(url.str());

        if (reply.str() != "ok")
            throw std::runtime_error("Expected reply 'ok' but got " + reply.str());
    }
    catch (std::exception &er)
    {
        Log(ERROR) << er.what();
    }
}

void postToPVOutput(const std::string &date, float dailyKWh, float dailyMax,
    uint32_t dailyMaxHour, uint32_t dailyMaxMinute)
{
    std::string url("https://pvoutput.org/service/r2/addoutput.jsp");
    std::list<std::string> header;

    header.push_back("X-Pvoutput-Apikey: c934fca74a5aed7e0ee536a2cd8c74850e0cd1dc");
    header.push_back("X-Pvoutput-SystemId: 59595");

    try
    {
        curlpp::Cleanup cleaner;
        curlpp::Easy request;

        request.setOpt(new curlpp::options::Url(url));
        request.setOpt(new curlpp::options::HttpHeader(header));

        std::ostringstream data;
        data << "data=" << date << "," << dailyKWh << "," << dailyKWh << "," << dailyMax << "," <<
            dailyMaxHour << ":" << dailyMaxMinute;

        request.setOpt(new curlpp::options::PostFields(data.str()));
        request.setOpt(new curlpp::options::PostFieldSize(data.str().length() + 1));

        std::ostringstream reply;
        reply << request;
        if (reply.str() != "OK 200: Added Output")
            throw std::runtime_error("Expected reply 'OK 200: Added Output' but got " + reply.str());
    }
    catch (std::exception &er)
    {
        Log(ERROR) << er.what();
    }
}
