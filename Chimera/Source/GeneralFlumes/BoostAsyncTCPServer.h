#pragma once
#include <winsock2.h>
#include <boost/asio.hpp>
#include <boost/thread.hpp>
#include <GeneralObjects/ThreadsafeQueue.h>

class TCPSession;

struct TCPMessageTyep {
    std::shared_ptr<TCPSession> connection;
    std::string msg;
};

class TCPSession : public std::enable_shared_from_this<TCPSession> {
public:
    TCPSession(boost::asio::ip::tcp::socket socket, ThreadsafeQueue<TCPMessageTyep>& queue);
    ~TCPSession();
    void start();

    void do_write(const std::string& msg);
    void do_write(const std::vector<char>& msg);

private:
    void do_read();

    boost::asio::ip::tcp::socket socket_;
    std::array<char, 1024> data_;
    ThreadsafeQueue<TCPMessageTyep>& queue_;
};

class BoostAsyncTCPServer {
public:
    BoostAsyncTCPServer(const std::string& address, short port);
    ~BoostAsyncTCPServer();
    ThreadsafeQueue<TCPMessageTyep>& dataQueue();
private:
    void do_accept();

    boost::asio::io_context io_context_;
    boost::asio::ip::tcp::acceptor acceptor_;
    boost::thread server_thread_;
    ThreadsafeQueue<TCPMessageTyep> queue_;
};