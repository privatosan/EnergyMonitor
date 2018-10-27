#include "Post.h"

#include <curlpp/cURLpp.hpp>
#include <curlpp/Options.hpp>

#include <sstream>

static const std::string IP_ADDR("192.168.178.25");

void post(const std::vector<Channel> &channels)
{
    std::ostringstream url;

    url << "http://" << IP_ADDR << "/emoncms/input/post.json?node=1&json={";

    bool addComma = false;
    for (auto&& channel : channels)
    {
        if (addComma)
            url << ",";
        url << channel.name() << ":" << channel.value();
        addComma = true;
    }
    url << "}&apikey=ab7fc3c6f67f0b0369cc1aca4afd9e4c";
    try
    {
        curlpp::Cleanup cleaner;

        std::ostringstream reply;
        reply << curlpp::options::Url(url.str());

        std::cout << reply.str() << std::endl;
    }
    catch (std::exception &er)
    {
        std::cerr << "Error: " << er.what() << std::endl;
    }
}
