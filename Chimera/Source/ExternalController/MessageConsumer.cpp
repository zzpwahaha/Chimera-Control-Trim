#include "stdafx.h"
#include "MessageConsumer.h"
#include "boost\lexical_cast.hpp"
#include <chrono>
#include <ctime>
#include <sstream>

MessageConsumer::MessageConsumer(ThreadsafeQueue<TCPMessageTyep>& queue, CommandModulator& modulator) : 
    queue_(queue),
    modulator_(modulator)
{}

MessageConsumer::~MessageConsumer()
{
    if (consumer_thread_.joinable()) {
        consumer_thread_.join();
    }
}

void MessageConsumer::start()
{
    consumer_thread_ = std::thread(&MessageConsumer::consume, this);
}

void MessageConsumer::consume()
{
    auto stratWith = [](std::string msg, std::string trait) {
        return msg.rfind(trait, 0) == 0; }; // pos=0 limits the search to the prefix
    while (true) {
        auto data = queue_.pop();
        auto& message = data.msg;
        auto& connection = data.connection;
        CommandModulator::ErrorStatus status;
        bool argValid = false;
        
        // Process the message
        std::string timeStamp = getCurrentDateTime();
        emit logMessage(qstr(timeStamp + ": " + data.msg));

        if (stratWith(message, "Error")) { 
            //emit error(qstr(message), 0);
            continue;
        }
        else if (stratWith(message, "Open-Configuration")) {
            auto args = getArguments(message, 1, argValid, connection);
            if (!argValid) continue;
            QMetaObject::invokeMethod(&modulator_, [&]() {
                modulator_.openConfiguration(qstr(args[0]), status);
                }, Qt::BlockingQueuedConnection);
            connection->do_write(compileReply("Finished opening configuration: " + args[0], status));
        }
        else if (stratWith(message, "Open-Master-Script")) {
            auto args = getArguments(message, 1, argValid, connection);
            if (!argValid) continue;
            QMetaObject::invokeMethod(&modulator_, [&]() {
                modulator_.openMasterScript(qstr(args[0]), status);
                }, Qt::BlockingQueuedConnection);
            connection->do_write(compileReply("Finished opening master script: " + args[0], status));

        }
        else if (stratWith(message, "Save-All")) {
            QMetaObject::invokeMethod(&modulator_, [&]() {
                modulator_.saveAll(status);
                }, Qt::BlockingQueuedConnection);
            connection->do_write(compileReply("Finished saving all", status));
        }
        else if (stratWith(message, "Start-Experiment")) {
            auto args = getArguments(message, 1, argValid, connection);
            if (!argValid) continue;
            QMetaObject::invokeMethod(&modulator_, [&]() {
                modulator_.startExperiment(qstr(args[0]), status);
                }, Qt::BlockingQueuedConnection);
            connection->do_write(compileReply("Finished starting experiment", status));
        }
        else if (stratWith(message, "Abort-Experiment")) {
            auto args = getArguments(message, 2, argValid, connection);
            if (!argValid) continue;
            QMetaObject::invokeMethod(&modulator_, [&]() {
                bool keepData = !stratWith(str(args[0]), "delete");
                modulator_.abortExperiment(keepData, qstr(args[1]), status);
                }, Qt::BlockingQueuedConnection);
            connection->do_write(compileReply("Finished aborting experiment", status));
        }
        else if (stratWith(message, "Is-Experiment-Running?")) {
            bool isRunning;
            QMetaObject::invokeMethod(&modulator_, [&]() {
                modulator_.isExperimentRunning(isRunning, status);
                }, Qt::BlockingQueuedConnection);
            std::string isRunningStr = isRunning ? "TRUE" : "FALSE";
            connection->do_write(compileReply(isRunningStr + "\tFinished asking if experiment is running", status));
        }
        else if (stratWith(message, "Start-Calibration")) {
            auto args = getArguments(message, 1, argValid, connection);
            if (!argValid) continue;
            QMetaObject::invokeMethod(&modulator_, [&]() {
                modulator_.startCalibration(qstr(args[0]), status);
                }, Qt::BlockingQueuedConnection);
            connection->do_write(compileReply("Finished starting calibration", status));
        }
        else if (stratWith(message, "Is-Calibration-Running?")) {
            bool isRunning;
            QMetaObject::invokeMethod(&modulator_, [&]() {
                modulator_.isCalibrationRunning(isRunning, status);
                }, Qt::BlockingQueuedConnection);
            std::string isRunningStr = isRunning ? "TRUE" : "FALSE";
            connection->do_write(compileReply(isRunningStr + "\tFinished asking if calibration is running", status));
        }
        else if (stratWith(message, "Set-Static-DDS")) {
            auto args = getArguments(message, 2, argValid, connection);
            if (!argValid) continue;
            QMetaObject::invokeMethod(&modulator_, [&]() {
                modulator_.setStaticDDS(qstr(args[0]), qstr(args[1]), status);
                }, Qt::BlockingQueuedConnection);
            connection->do_write(compileReply("Finished setting static DDS", status));
        }
        else if (stratWith(message, "Set-TTL")) {
            auto args = getArguments(message, 2, argValid, connection);
            if (!argValid) continue;
            QMetaObject::invokeMethod(&modulator_, [&]() {
                modulator_.setTTL(qstr(args[0]), qstr(args[1]), status);
                }, Qt::BlockingQueuedConnection);
            connection->do_write(compileReply("Finished setting DAC", status));
        }
        else if (stratWith(message, "Set-DAC")) {
            auto args = getArguments(message, 2, argValid, connection);
            if (!argValid) continue;
            QMetaObject::invokeMethod(&modulator_, [&]() {
                modulator_.setDAC(qstr(args[0]), qstr(args[1]), status);
                }, Qt::BlockingQueuedConnection);
            connection->do_write(compileReply("Finished setting DAC", status));
        }
        else if (stratWith(message, "Set-DDS")) {
            QMetaObject::invokeMethod(&modulator_, [&]() {
                modulator_.setDDS(status);
                }, Qt::BlockingQueuedConnection);
            connection->do_write(compileReply("Finished setting DDS", status));
        }
        else if (stratWith(message, "Set-OL")) {
            QMetaObject::invokeMethod(&modulator_, [&]() {
                modulator_.setOL(status);
                }, Qt::BlockingQueuedConnection);
            connection->do_write(compileReply("Finished setting OL", status));
        }
        else if (stratWith(message, "Start-MAKO")) {
            auto args = getArguments(message, 1, argValid, connection);
            if (!argValid) continue;
            QMetaObject::invokeMethod(&modulator_, [&]() {
                modulator_.startMako(qstr(args[0]), status);
                }, Qt::BlockingQueuedConnection);
            connection->do_write(compileReply("Finished starting MAKO", status));
        }
        else if (stratWith(message, "Stop-MAKO")) {
            auto args = getArguments(message, 1, argValid, connection);
            if (!argValid) continue;
            QMetaObject::invokeMethod(&modulator_, [&]() {
                modulator_.stopMako(qstr(args[0]), status);
                }, Qt::BlockingQueuedConnection);
            connection->do_write(compileReply("Finished starting MAKO", status));
        }
        else if (stratWith(message, "Get-MAKO-Image")) {
            auto args = getArguments(message, 1, argValid, connection);
            if (!argValid) continue;
            QVector<char> image;
            QMetaObject::invokeMethod(&modulator_, [&]() {
                modulator_.getMakoImage(qstr(args[0]), image, status);
                }, Qt::BlockingQueuedConnection);
            connection->do_write(compileReply("Finished getting MAKO image", image.toStdVector(), status));
        }
        else if (stratWith(message, "Get-MAKO-Dimension")) {
            auto args = getArguments(message, 1, argValid, connection);
            if (!argValid) continue;
            QVector<char> imageDim;
            QMetaObject::invokeMethod(&modulator_, [&]() {
                modulator_.getMakoImageDimension(qstr(args[0]), imageDim, status);
                }, Qt::BlockingQueuedConnection);
            connection->do_write(compileReply("Finished getting MAKO image dimension", imageDim.toStdVector(), status));
        }
        else if (stratWith(message, "Get-MAKO-Feature-Value")) {
            auto args = getArguments(message, 3, argValid, connection);
            if (!argValid) continue;
            QVector<char> featureValue;
            QMetaObject::invokeMethod(&modulator_, [&]() {
                modulator_.getMakoFeatureValue(qstr(args[0]), qstr(args[1]), qstr(args[2]), featureValue, status);
                }, Qt::BlockingQueuedConnection);
            connection->do_write(compileReply("Finished getting MAKO feature value", featureValue.toStdVector(), status));
        }
        else if (stratWith(message, "Set-MAKO-Feature-Value")) {
            auto args = getArguments(message, 4, argValid, connection);
            if (!argValid) continue;
            QVector<char> featureValue;
            QMetaObject::invokeMethod(&modulator_, [&]() {
                modulator_.setMakoFeatureValue(qstr(args[0]), qstr(args[1]), qstr(args[2]), qstr(args[3]), status);
                }, Qt::BlockingQueuedConnection);
            connection->do_write(compileReply("Finished setting MAKO feature value", status));
        }
        else if (stratWith(message, "Set-PicoScrew-Position")) {
            auto args = getArguments(message, 3, argValid, connection);
            if (!argValid) continue;
            QMetaObject::invokeMethod(&modulator_, [&]() {
                modulator_.setPicoScrewPosition(qstr(args[0]), qstr(args[1]), qstr(args[2]), status);
                }, Qt::BlockingQueuedConnection);
            connection->do_write(compileReply("Finished setting PicoScrew position", status));
        }
        else if (stratWith(message, "Get-PicoScrew-Positions")) {
            QVector<char> positionValue;
            QMetaObject::invokeMethod(&modulator_, [&]() {
                modulator_.getPicoScrewPositions(positionValue, status);
                }, Qt::BlockingQueuedConnection);
            connection->do_write(compileReply("Finished getting PicoScrew position", positionValue.toStdVector(), status));
        }
        else if (stratWith(message, "Set-PicoScrew-Homes")) {
            QMetaObject::invokeMethod(&modulator_, [&]() {
                modulator_.setPicoScrewHomes(status);
                }, Qt::BlockingQueuedConnection);
            connection->do_write(compileReply("Finished setting Home for all screws", status));
        }
        else {
            emit logMessage(qstr(timeStamp + ": \t" + "Unrecongnized command: " + message));
            connection->do_write("Error\nUnrecongnized command: " + message);
        }
    }
}

std::string MessageConsumer::compileReply(std::string normalMsg, CommandModulator::ErrorStatus status)
{
    if (status.error) {
        return str("Error\n") + status.errorMsg;
    }
    else {
        return normalMsg;
    }
}

std::vector<char> MessageConsumer::compileReply(std::string normalMsg, std::vector<char> data, CommandModulator::ErrorStatus status)
{
    if (status.error) {
        std::string errorMessage = str("Error\n") + status.errorMsg;
        return std::vector<char>(errorMessage.begin(), errorMessage.end());
    }
    std::vector<char> buffer; // normalMsg + delimiter($) + size of vector + vector

    std::size_t size = data.size();
    buffer.reserve(normalMsg.size() + 1 + sizeof(size) + sizeof(char) * size);
    // Copy the message and add the delimiter
    buffer.insert(buffer.end(), normalMsg.begin(), normalMsg.end());
    buffer.push_back(delimiter_);
    buffer.insert(buffer.end(), data.begin(), data.end());
    return buffer;
}

std::string MessageConsumer::getCurrentDateTime()
{
    // Get current time as a time_point
    auto now = std::chrono::system_clock::now();
    // Convert to time_t, which represents time in seconds since epoch
    std::time_t now_time = std::chrono::system_clock::to_time_t(now);
    // Convert to tm structure for local time
    std::tm now_tm = *std::localtime(&now_time);
    // Create a stringstream to format the date and time
    std::ostringstream oss;
    oss << std::put_time(&now_tm, "%Y-%m-%d %H:%M:%S");
    return oss.str();
}

std::vector<std::string> MessageConsumer::getArguments(const std::string& command)
{
    auto result = splitString(command, delimiter_);
    // remove the first one since that is the command itself
    if (!result.empty()) {
        result.erase(result.begin());
    }
    return result;
}

std::vector<std::string> MessageConsumer::getArguments(const std::string& command, unsigned expectNumArg, bool& success, std::shared_ptr<TCPSession> connection)
{
    auto args = getArguments(command);
    if (args.size() != expectNumArg) {
        emit logMessage(qstr(getCurrentDateTime() + ": \t" + "Not enough arguemnt found in command: " + command + ". Expecting " + str(expectNumArg) + ", but only get " + str(args.size())));
        connection->do_write("Error\nNot enough arguemnt found in command: " + command + ". Expecting " +str(expectNumArg) + ", but only get " + str(args.size()));
        success = false;
    }
    success = true;
    return args;
}

std::vector<std::string> MessageConsumer::splitString(const std::string& input, char delimiter)
{
    std::vector<std::string> substrings;
    std::stringstream ss(input);
    std::string item;

    while (std::getline(ss, item, delimiter)) {
        substrings.push_back(item);
    }

    return substrings;
}

std::vector<std::string> MessageConsumer::splitString(const std::string& input, const std::string& delimiter)
{
    std::vector<std::string> substrings;
    std::string::size_type start = 0;
    std::string::size_type end;

    while ((end = input.find(delimiter, start)) != std::string::npos) {
        substrings.push_back(input.substr(start, end - start));
        start = end + delimiter.length();
    }
    // Add the last substring
    substrings.push_back(input.substr(start));

    return substrings;
}
