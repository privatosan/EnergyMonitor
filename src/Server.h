#ifndef SERVER_H
#define SERVER_H

#include <memory>
#include <vector>

class ChannelAD;

class Server
{
public:
    static Server &getInstance();

    void update(const std::vector<const ChannelAD*> &channels);

    void shutdown();

private:
    // this is a singleton, hide copy constructor etc.
    Server(const Server&);
    Server& operator=(const Server&);
    // a singleton constructor/destructor is not thread safe and need to be empty
    Server() { };
    ~Server() { };

    void initialize();

    struct Impl;
    std::shared_ptr<Impl> m_impl;
};

#endif // SERVER_H
