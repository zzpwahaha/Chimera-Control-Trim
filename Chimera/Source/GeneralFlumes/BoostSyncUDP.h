#pragma once
#include <winsock2.h>
#include <boost/asio.hpp>

class BoostSyncUDP {
public:
    // THIS CLASS IS NOT COPYABLE.
    // Delete copy constructor and copy assignment operator
    BoostSyncUDP(const BoostSyncUDP&) = delete;
    BoostSyncUDP& operator=(const BoostSyncUDP&) = delete;
    // Default move constructor and move assignment operator
    BoostSyncUDP(BoostSyncUDP&&) noexcept = default;
    BoostSyncUDP& operator=(BoostSyncUDP&&) noexcept = default;

    BoostSyncUDP(bool safemode, const std::string& host, const std::string& port);
    BoostSyncUDP(bool safemode, const std::string& host, int port);
    ~BoostSyncUDP();

    // Connects to the given host and port (UDP does not actually establish a connection, but for consistency)
    void connect(const std::string& host, const std::string& port);
    void connect(const std::string& host, int port);
    void disconnect();
    void write(const std::string& message);
    void write(const std::vector<unsigned char>& data);
    std::string read();

private:
    static std::string port_to_string(int port);

private:
    const bool safemode;
    const std::string host;
    const std::string port;

    boost::asio::io_context io_context_;      // IO context for asynchronous operations
    boost::asio::ip::udp::socket socket_;     // UDP socket for communication
    boost::asio::ip::udp::endpoint remote_endpoint_; // Endpoint for the remote server
};
