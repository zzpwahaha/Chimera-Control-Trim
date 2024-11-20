#pragma once
#include <qobject.h>
#include <thread>
#include <GeneralFlumes/BoostAsyncTCPServer.h>
#include <ExternalController/CommandModulator.h>

class MessageConsumer : public QObject
{
    Q_OBJECT
public:
    // THIS CLASS IS NOT COPYABLE.
    MessageConsumer& operator=(const MessageConsumer&) = delete;
    MessageConsumer(const MessageConsumer&) = delete;
    MessageConsumer(ThreadsafeQueue<TCPMessageTyep>& queue, CommandModulator& modulator);
    ~MessageConsumer();
    void start();
private:
    void consume();
    std::string compileReply(std::string normalMsg, CommandModulator::ErrorStatus status);
    std::vector<char> compileReply(std::string normalMsg, std::vector<char> data, CommandModulator::ErrorStatus status);
    std::string getCurrentDateTime();
    std::vector<std::string> getArguments(const std::string& command);
    std::vector<std::string> getArguments(const std::string& command, unsigned expectNumArg, bool& success, std::shared_ptr<TCPSession>connection);
    std::vector<std::string> splitString(const std::string& input, char delimiter);
    std::vector<std::string> splitString(const std::string& input, const std::string& delimiter);

signals:
    void logMessage(QString msg);
    //void error(QString msg, unsigned errorLevel = 0);

private:
    const char delimiter_ = '$';
    ThreadsafeQueue<TCPMessageTyep>& queue_;
    std::thread consumer_thread_;
    CommandModulator& modulator_;
};

