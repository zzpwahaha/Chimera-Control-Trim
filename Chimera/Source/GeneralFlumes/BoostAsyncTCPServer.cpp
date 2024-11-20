#include "stdafx.h"
#include "BoostAsyncTCPServer.h"

TCPSession::TCPSession(boost::asio::ip::tcp::socket socket, ThreadsafeQueue<TCPMessageTyep>& queue) :
    socket_(std::move(socket)),
    queue_(queue)
{}

TCPSession::~TCPSession()
{
}

void TCPSession::start()
{
    do_read();
}

void TCPSession::do_write(const std::string& msg)
{
    auto self(shared_from_this());
    auto buffer = std::make_shared<std::string>(msg);
    boost::asio::async_write(socket_, boost::asio::buffer(*buffer),
        [this, self, buffer](boost::system::error_code ec, std::size_t /*length*/) {
            // The lambda handler captures the std::shared_ptr by value, 
            // extending the lifetime of the buffer until the asynchronous operation completes (handler get called).
            if (!ec) {}
            else {
                std::string err = "Error in writing data to socket with message: " + *buffer +
                    ". And the error message is " + ec.message();
                queue_.push({ self, err }); // the callback happen in the server thread, where the TCPSession is created
            }
        });
}

void TCPSession::do_write(const std::vector<char>& msg)
{
    auto self(shared_from_this());
    auto buffer = std::make_shared<std::vector<char>>(msg);
    boost::asio::async_write(socket_, boost::asio::buffer(*buffer),
        [this, self, buffer](boost::system::error_code ec, std::size_t /*length*/) {
            // The lambda handler captures the std::shared_ptr by value, 
            // extending the lifetime of the buffer until the asynchronous operation completes (handler get called).
            if (!ec) {}
            else {
                std::string err = "Error in writing data to socket with message: " + std::string(buffer->begin(), buffer->end()); +
                    ". And the error message is " + ec.message();
                queue_.push({ self, err }); // the callback happen in the server thread, where the TCPSession is created
            }
        });
}

void TCPSession::do_read()
{
    auto self(shared_from_this());
    socket_.async_read_some(boost::asio::buffer(data_),
        [this, self](boost::system::error_code ec, std::size_t length) {
            if (!ec) {
                queue_.push({ self, std::string(data_.begin(), data_.begin() + length) });
                do_read();
            }
            else {
                std::string err = "Error in reading data from socket. And the error message is " + ec.message();
                queue_.push({ self, err }); // the callback happen in the server thread, where the TCPSession is created
            }
        });
}

BoostAsyncTCPServer::BoostAsyncTCPServer(const std::string& address, short port) :
    acceptor_(io_context_, boost::asio::ip::tcp::endpoint(boost::asio::ip::make_address(address), port))
{
    server_thread_ = boost::thread([this]() { io_context_.run(); });
    do_accept();
}

BoostAsyncTCPServer::~BoostAsyncTCPServer()
{
    io_context_.stop();
    if (server_thread_.joinable()) {
        server_thread_.join();
    }
}

ThreadsafeQueue<TCPMessageTyep>& BoostAsyncTCPServer::dataQueue()
{
    return queue_;
}

void BoostAsyncTCPServer::do_accept()
{
    acceptor_.async_accept(
        [this](boost::system::error_code ec, boost::asio::ip::tcp::socket socket) {
            if (!ec) {
                std::make_shared<TCPSession>(std::move(socket), queue_)->start();
            }
            do_accept();
        });
}