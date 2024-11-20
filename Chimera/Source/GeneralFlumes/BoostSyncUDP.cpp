#include "stdafx.h"
#include "BoostSyncUDP.h"
#include <iostream>
#include <sstream>
#include <boost/asio/ip/udp.hpp>

BoostSyncUDP::BoostSyncUDP(bool safemode, const std::string& host, const std::string& port)
    : safemode(safemode), host(host), port(port), io_context_(), socket_(io_context_) {
    connect(host, port);
}

BoostSyncUDP::BoostSyncUDP(bool safemode, const std::string& host, int port)
    : safemode(safemode), host(host), port(port_to_string(port)), io_context_(), socket_(io_context_) {
    connect(host, port);
}

BoostSyncUDP::~BoostSyncUDP() {
    disconnect();
}

std::string BoostSyncUDP::port_to_string(int port) {
    std::ostringstream oss;
    oss << port;
    return oss.str();
}

// Connect method (binds the socket and prepares for communication)
void BoostSyncUDP::connect(const std::string& host, const std::string& port) {
    using boost::asio::ip::udp;

    udp::resolver resolver(io_context_);
    udp::resolver::results_type endpoints = resolver.resolve(udp::v4(), host, port);
    remote_endpoint_ = *endpoints.begin(); // Use the first resolved endpoint

    socket_.open(udp::v4());

    std::cout << "Connected to " << host << ":" << port << std::endl;
}

// Overloaded connect method (with int port)
void BoostSyncUDP::connect(const std::string& host, int port) {
    connect(host, port_to_string(port));
}

// Disconnect method (not strictly necessary for UDP but included for consistency)
void BoostSyncUDP::disconnect() {
    if (socket_.is_open()) {
        socket_.close();
        std::cout << "Disconnected" << std::endl;
    }
}

void BoostSyncUDP::write(const std::string& message) {
    socket_.send_to(boost::asio::buffer(message), remote_endpoint_);
}

void BoostSyncUDP::write(const std::vector<unsigned char>& data) {
    socket_.send_to(boost::asio::buffer(data), remote_endpoint_);
}

std::string BoostSyncUDP::read() {
    char buffer[1024]; // Adjust buffer size as needed
    boost::asio::ip::udp::endpoint sender_endpoint;
    std::size_t length = socket_.receive_from(boost::asio::buffer(buffer), sender_endpoint);

    return std::string(buffer, length);
}
