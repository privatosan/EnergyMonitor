#include "Server.h"
#include "Channel.h"

#include <thread>
#include <memory>
#include <mutex>
#include <list>
#include <set>
#include <mutex>
#include <array>
#include <vector>

#include <nlohmann/json.hpp>

// The ASIO_STANDALONE define is necessary to use the standalone version of Asio.
// Remove if you are using Boost Asio.
#define ASIO_STANDALONE

#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/server.hpp>

using WebServer = websocketpp::server<websocketpp::config::asio>;

class Server::Impl
{
public:
    void threadLoop();

    void openHandler(websocketpp::connection_hdl hdl);
    void closeHandler(websocketpp::connection_hdl hdl);
    void messageHandler(websocketpp::connection_hdl hdl, WebServer::message_ptr msg);

    WebServer m_endpoint;

    std::mutex m_connectionsMutex;
    typedef std::set<websocketpp::connection_hdl, std::owner_less<websocketpp::connection_hdl>> ConnList;
    ConnList m_connections;

    std::unique_ptr<std::thread> m_thread;

    std::mutex m_mutex;
    std::list<ChannelAD> m_channels;
};

Server &Server::getInstance()
{
    static std::once_flag onceFlag;
    static Server instance;

    std::call_once(onceFlag, [] {
        instance.initialize();
    });

    return instance;
}

void Server::initialize()
{
    m_impl.reset(new Impl);

    // Set logging settings
    m_impl->m_endpoint.set_error_channels(websocketpp::log::elevel::all);
    m_impl->m_endpoint.set_access_channels(websocketpp::log::alevel::none);
    // see https://github.com/zaphoyd/websocketpp/issues/803#issuecomment-912953104
    m_impl->m_endpoint.set_reuse_addr(true);

    // Initialize Asio
    m_impl->m_endpoint.init_asio();

    // Listen on port 9002
    m_impl->m_endpoint.listen(9002);

    // Queues a connection accept operation
    m_impl->m_endpoint.start_accept();

    // set open and close handlers to track connections
    m_impl->m_endpoint.set_open_handler(std::bind(
        &Server::Impl::openHandler, m_impl.get(),
        std::placeholders::_1));
    m_impl->m_endpoint.set_close_handler(std::bind(
        &Server::Impl::closeHandler, m_impl.get(),
        std::placeholders::_1));

    // Set the default message handler to the echo handler
    m_impl->m_endpoint.set_message_handler(std::bind(
        &Server::Impl::messageHandler, m_impl.get(),
        std::placeholders::_1, std::placeholders::_2
    ));

    m_impl->m_thread.reset(new std::thread(&Server::Impl::threadLoop, m_impl.get()));
}


void Server::shutdown()
{
    if (m_impl)
    {
        m_impl->m_endpoint.stop_listening();
        for (auto it = m_impl->m_connections.begin(); it != m_impl->m_connections.end(); ++it)
        {
            m_impl->m_endpoint.get_con_from_hdl(*it)->close(websocketpp::close::status::going_away, "Server going down");
        }

        m_impl->m_thread->join();

        m_impl.reset();
    }
}

void Server::Impl::openHandler(websocketpp::connection_hdl hdl)
{
    std::lock_guard<std::mutex> lock(m_connectionsMutex);
    m_connections.insert(hdl);
}

void Server::Impl::closeHandler(websocketpp::connection_hdl hdl)
{
    std::lock_guard<std::mutex> lock(m_connectionsMutex);
    m_connections.erase(hdl);
}

void Server::Impl::messageHandler(websocketpp::connection_hdl hdl, WebServer::message_ptr msg)
{
    try
    {
        const nlohmann::json json = nlohmann::json::parse(msg->get_payload());
        nlohmann::json payload;

        const std::string type = json.at("type");
        if (type == "GetNames")
        {
            std::vector<std::string> names;
            {
                std::unique_lock<std::mutex> lock(m_mutex);

                for (auto &&channel: m_channels)
                {
                    names.push_back(channel.name());
                }
            }

            payload["type"] = "Names";
            payload["data"]["names"] = nlohmann::json(names);
        }
        else if (type == "GetValues")
        {
            const std::string name = json.at("data").at("name");
            payload["type"] = "Values";
            payload["data"]["name"] = name;

            std::vector<std::array<float, 2>> values;
            {
                std::unique_lock<std::mutex> lock(m_mutex);

                for (auto &&channel: m_channels)
                {
                    if (channel.name() == name)
                    {
                        for (size_t index = 0; index < channel.sampleCount(); ++index)
                        {
                            const std::array<float, 2> sample = { { static_cast<float>(channel.sampleTime(index) - channel.sampleTime(0)), channel.value(index) } };
                            values.emplace_back(sample);
                        }
                        break;
                    }
                }
            }
            payload["data"]["values"] = values;
        }
        else
        {
            std::stringstream error;
            error << "Unhandled message of type " << json.at("type");
            throw std::runtime_error(error.str());
        }
        // write a new message
        m_endpoint.send(hdl, payload.dump(), websocketpp::frame::opcode::value::text);
    }
    catch(std::exception &e)
    {
        Log(ERROR) << "Message handler error " << e.what();
    }
}

void Server::Impl::threadLoop()
{
    // Start the Asio io_service run loop
    m_endpoint.run();
}

void Server::update(const std::vector<const ChannelAD*> &channels)
{
    std::unique_lock<std::mutex> lock(m_impl->m_mutex);

    m_impl->m_channels.clear();
    for (auto &&channel: channels)
    {
        m_impl->m_channels.emplace_back(*channel);
    }
}
