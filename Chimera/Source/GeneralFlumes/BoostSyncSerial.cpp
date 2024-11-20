#include "stdafx.h"
#include "BoostSyncSerial.h"

BoostSyncSerial::BoostSyncSerial(const std::string& port, unsigned int baud_rate,
    unsigned int char_size,
    boost::asio::serial_port_base::stop_bits::type stop_bits,
    boost::asio::serial_port_base::parity::type parity,
    boost::asio::serial_port_base::flow_control::type flow_control) : 
    portID(port), 
    io(),
    serial(io),
    timer(io)
{
    open();
    setupSerial(baud_rate, char_size, stop_bits, parity, flow_control);
}

BoostSyncSerial::BoostSyncSerial(const std::string& port, unsigned int baud_rate)
    : BoostSyncSerial(port, baud_rate, 8U,
        boost::asio::serial_port_base::stop_bits::one,
        boost::asio::serial_port_base::parity::none,
        boost::asio::serial_port_base::flow_control::none) {}

BoostSyncSerial::~BoostSyncSerial() {
    disconnect();
}
void BoostSyncSerial::write(const std::string& data) {
    boost::asio::write(serial, boost::asio::buffer(data));
}

std::string BoostSyncSerial::read(size_t size)
{
    std::string result(size, '\0');
    boost::asio::read(serial, boost::asio::buffer(&result[0], size));
    return result;
}

std::string BoostSyncSerial::readUntil(char delimiter)
{
    std::string result;
    char ch;
    while (true) {
        boost::asio::read(serial, boost::asio::buffer(&ch, 1));
        result += ch;

        if (ch == delimiter) {
            break;
        }
    }
    return result;
}

/// below probably wouldn't work since this is synchronous reading, the io_context is not run
//std::string BoostSyncSerial::read(size_t size, int timeout_seconds) {
//    std::string result(size, '\0');
//    char* data_ptr = &result[0];
//    size_t bytes_read = 0;
//    bool time_out_flag = false;
//
//    // Set up the timer
//    timer.expires_from_now(boost::posix_time::seconds(timeout_seconds));
//    timer.async_wait([this, &time_out_flag](const boost::system::error_code& error) {
//        if (!error) {
//            time_out_flag = true; // Timer expired
//        }
//        });
//
//    // Start the read operation
//    while (bytes_read < size) {
//        try {
//            bytes_read += boost::asio::read(serial, boost::asio::buffer(data_ptr + bytes_read, size - bytes_read));
//        }
//        catch (const std::exception& e) {
//            timer.cancel(); // Cancel the timer if there's an error
//            throwNested("Error in reading synchronous serial on port: " + portID);
//        }
//
//        if (time_out_flag) {
//            thrower("Read operation timed out for synchronous serial on port: " + portID);
//        }
//    }
//
//    timer.cancel(); // Cancel the timer if read was successful
//    return result;
//}
//
//std::string BoostSyncSerial::readUntil(char delimiter, int timeout_seconds) {
//    std::string result;
//    char ch;
//    bool time_out_flag = false;
//
//    // Set up the timer
//    timer.expires_from_now(boost::posix_time::seconds(timeout_seconds));
//    timer.async_wait([this, &time_out_flag](const boost::system::error_code& error) {
//        if (!error) {
//            time_out_flag = true; // Timer expired
//        }
//        });
//
//    while (true) {
//        try {
//            boost::asio::read(serial, boost::asio::buffer(&ch, 1));
//        }
//        catch (const std::exception& e) {
//            timer.cancel(); // Cancel the timer if there's an error
//            throwNested("Error in reading synchronous serial on port: " + portID);
//        }
//        result += ch;
//        if (ch == delimiter) {
//            break;
//        }
//        if (time_out_flag) {
//            thrower("Read operation timed out for synchronous serial on port: " + portID);
//        }
//    }
//    timer.cancel(); // Cancel the timer if read was successful
//    return result;
//}

void BoostSyncSerial::open()
{
    if (!serial.is_open()) {
        serial.open(portID);
    }
}

void BoostSyncSerial::disconnect()
{
    if (serial.is_open()) {
        serial.close();
    }
}

void BoostSyncSerial::reconnect()
{
    disconnect();
    open();
}

void BoostSyncSerial::setupSerial(unsigned int baud_rate, unsigned int char_size,
    boost::asio::serial_port_base::stop_bits::type stop_bits,
    boost::asio::serial_port_base::parity::type parity,
    boost::asio::serial_port_base::flow_control::type flow_control) {
    serial.set_option(boost::asio::serial_port_base::baud_rate(baud_rate));
    serial.set_option(boost::asio::serial_port_base::character_size(char_size));
    serial.set_option(boost::asio::serial_port_base::stop_bits(stop_bits));
    serial.set_option(boost::asio::serial_port_base::parity(parity));
    serial.set_option(boost::asio::serial_port_base::flow_control(flow_control));
}
