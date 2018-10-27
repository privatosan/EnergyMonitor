#include "Post.h"
#include "Options.h"
#include "Log.h"

#include <curlpp/cURLpp.hpp>
#include <curlpp/Options.hpp>

#include <sstream>

static const std::string IP_ADDR("192.168.178.25");

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
