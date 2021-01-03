#include "Server.h"
#include "Channel.h"

#include <thread>
#include <memory>

// The ASIO_STANDALONE define is necessary to use the standalone version of Asio.
// Remove if you are using Boost Asio.
#define ASIO_STANDALONE

#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/server.hpp>

using WebServer = websocketpp::server<websocketpp::config::asio>;

class Server::Impl
{
public:
    WebServer m_endpoint;
    std::unique_ptr<std::thread> m_thread;
    std::vector<const Channel*> m_channels;
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
    m_impl->m_endpoint.set_access_channels(websocketpp::log::alevel::all ^ websocketpp::log::alevel::frame_payload);

    // Initialize Asio
    m_impl->m_endpoint.init_asio();

    // Listen on port 9002
    m_impl->m_endpoint.listen(9002);

    // Queues a connection accept operation
    m_impl->m_endpoint.start_accept();

    m_impl->m_thread.reset(new std::thread(&Server::threadLoop, this));


#if 0
    {
        // Set the default message handler to the echo handler
        m_endpoint.set_message_handler(std::bind(
            &utility_server::echo_handler, this,
            std::placeholders::_1, std::placeholders::_2
        ));
    }

    void echo_handler(websocketpp::connection_hdl hdl, WebServer::message_ptr msg) {
        // write a new message
        m_endpoint.send(hdl, msg->get_payload(), msg->get_opcode());
    }
#endif
}

void Server::threadLoop()
{
    // Start the Asio io_service run loop
    m_impl->m_endpoint.run();
}

void Server::update(const std::vector<const Channel*> &channels)
{
    m_impl->m_channels = channels;
}
