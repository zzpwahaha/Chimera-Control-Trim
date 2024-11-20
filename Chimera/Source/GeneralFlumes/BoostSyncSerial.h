#pragma once
#include <winsock2.h>
#include <boost/asio.hpp>
#include <string>

class BoostSyncSerial
{
public:
    // THIS CLASS IS NOT COPYABLE.
    BoostSyncSerial& operator=(const BoostSyncSerial&) = delete;
    BoostSyncSerial(const BoostSyncSerial&) = delete;

    BoostSyncSerial(const std::string& port, unsigned int baud_rate,
        unsigned int char_size,
        boost::asio::serial_port_base::stop_bits::type stop_bits,
        boost::asio::serial_port_base::parity::type parity,
        boost::asio::serial_port_base::flow_control::type flow_control);
    BoostSyncSerial(const std::string& port, unsigned int baud_rate);
    ~BoostSyncSerial();

    void write(const std::string& data);
    std::string read(size_t size); 
    std::string readUntil(char delimiter); 

    void open();
    void disconnect(); // Rename close to disconnect
    void reconnect(); // Rename reopen to reconnect

    const std::string portID;

private:
    void setupSerial(unsigned int baud_rate, unsigned int char_size,
        boost::asio::serial_port_base::stop_bits::type stop_bits,
        boost::asio::serial_port_base::parity::type parity,
        boost::asio::serial_port_base::flow_control::type flow_control);

    boost::asio::io_context io;
    boost::asio::serial_port serial;
    boost::asio::deadline_timer timer; // Timer for read timeout
};

