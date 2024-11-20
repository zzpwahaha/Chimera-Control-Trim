#include "stdafx.h"
#include "BoostSyncTCP.h"

BoostSyncTCP::BoostSyncTCP(bool safemode, const std::string& host, const std::string& port)
    : safemode(safemode)
    , host(host)
    , port(port)
    , io_context_()
    , socket_(io_context_) 
{
    if (!safemode) {
        connect(host, port);
    }
}

BoostSyncTCP::BoostSyncTCP(bool safemode, const std::string& host, int port)
    : BoostSyncTCP(safemode, host, port_to_string(port))
{}

BoostSyncTCP::~BoostSyncTCP() {
    disconnect();
}

void BoostSyncTCP::connect(const std::string& host, const std::string& port) {
    using boost::asio::ip::tcp;

    tcp::resolver resolver(io_context_);
    tcp::resolver::results_type endpoints = resolver.resolve(host, port);
    boost::asio::connect(socket_, endpoints);

    std::cout << "Connected to " << host << ":" << port << std::endl;
}

void BoostSyncTCP::disconnect() {
    if (socket_.is_open()) {
        socket_.close();
        std::cout << "Disconnected" << std::endl;
    }
}

void BoostSyncTCP::write(const std::string& message) {
    if (safemode) {
        return;
    }
    boost::asio::write(socket_, boost::asio::buffer(message));
}

void BoostSyncTCP::write(const std::vector<unsigned char>& data) {
    if (safemode) {
        return;
    }
    boost::asio::write(socket_, boost::asio::buffer(data));
}

std::string BoostSyncTCP::read() {
    if (safemode) {
        return "SAFEMODE";
    }
    boost::asio::streambuf buffer;
    boost::asio::read_until(socket_, buffer, '\n');

    std::istream input_stream(&buffer);
    std::string message;
    std::getline(input_stream, message);

    return message;
}

std::string BoostSyncTCP::port_to_string(int port)
{
    std::ostringstream oss;
    oss << port;
    return oss.str();
}
