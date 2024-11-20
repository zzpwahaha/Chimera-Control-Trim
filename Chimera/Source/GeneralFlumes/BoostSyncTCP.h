#pragma once
#include <winsock2.h>
#include <boost/asio.hpp>

class BoostSyncTCP {
public:
    // THIS CLASS IS NOT COPYABLE.
    // Delete copy constructor and copy assignment operator
    BoostSyncTCP(const BoostSyncTCP&) = delete;
    BoostSyncTCP& operator=(const BoostSyncTCP&) = delete;
    // Default move constructor and move assignment operator
    BoostSyncTCP(BoostSyncTCP&&) noexcept = default;
    BoostSyncTCP& operator=(BoostSyncTCP&&) noexcept = default;

    BoostSyncTCP(bool safemode, const std::string& host, const std::string& port);
    BoostSyncTCP(bool safemode, const std::string& host, int port);
    ~BoostSyncTCP();

    void connect(const std::string& host, const std::string& port);
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
    boost::asio::ip::tcp::socket socket_;     // TCP socket for communication
};

