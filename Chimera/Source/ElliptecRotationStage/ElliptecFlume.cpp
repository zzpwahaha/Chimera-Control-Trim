#include "stdafx.h"
#include "ElliptecFlume.h"

ElliptecFlume::ElliptecFlume(std::string portAddress, bool safemode)
	: boostFlume(safemode, portAddress, 9600)
	, SAFEMODE(safemode)
	, readComplete(true)
{
	boostFlume.setReadCallback(boost::bind(&ElliptecFlume::readCallback, this, _1));
	boostFlume.setErrorCallback(boost::bind(&ElliptecFlume::errorCallback, this, _1));
}

std::string ElliptecFlume::query(std::string msg)
{
	write(msg);
	return read();
}

std::string ElliptecFlume::queryWithCheck(std::string msg)
{
	write(msg);
	return readWithCheck();
}

void ElliptecFlume::write(std::string msg)
{
	readRegister.clear();
	errorMsg.clear();
	readComplete = false;
	/*write data to serial port*/
	std::vector<unsigned char> byteMsg(msg.cbegin(), msg.cend());
	boostFlume.write(byteMsg);
	/*check exception after write*/
	if (auto e = boostFlume.lastException()) {
		try {
			boost::rethrow_exception(e);
		}
		catch (boost::system::system_error& e) {
			throwNested("Error seen in writing to serial port " + str(boostFlume.portID) + ". Error: " + e.what());
		}
	}
}

std::string ElliptecFlume::readWithCheck()
{
	auto message = read();
	auto channel = message.substr(0, 1); // first char represents different channel
	auto header = message.substr(1, 2); // take the header to see if it is GS, 
	if (header == "GS") {
		auto result = message.substr(3, 2); // take the returned error code
		int errorCode = std::stoi(result);
		if (errorCode != 0) {
			auto errorMsg = handleError(errorCode);
			thrower("Error in message from Elliptec rotation stage, channel number: "
				+ channel + ". Error message: " + errorMsg);
		}
	}
	return message;
}

std::string ElliptecFlume::queryIdentity(unsigned identifier)
{
	auto idn = queryWithCheck(str(identifier) + "in");
	return idn.substr(3, 14);
}

void ElliptecFlume::resetConnection()
{
	boostFlume.disconnect();
	Sleep(10);
	boostFlume.reconnect();
}

// This should be called with a expectation of reading something, e.g. after writing and that is why the reading register etc is not initialized. 
// Otherwise it will throw
std::string ElliptecFlume::read()
{
	if (SAFEMODE) {
		return std::string("Elliptec is in safemode.");
	}
	std::string recv;
	/*read register after write*/
	for (auto idx : range(1500)) {
		if (readComplete) {
			recv = std::string(readRegister.cbegin(), readRegister.cend());
			break;
		}
		Sleep(1);
	}
	/*check reading error and reading result and if reading is complete*/
	if (recv.empty() || !readComplete) {
		thrower("Reading is empty and timed out for 1500ms in reading from Elliptec serial port " + str(boostFlume.portID) + ".");
	}
	if (!errorMsg.empty()) {
		thrower("Nothing feeded back from Elliptec, something might be wrong with it." + recv + "\r\nError message: " + errorMsg);
	}
	recv.erase(std::remove(recv.begin(), recv.end(), '\n'), recv.end());
	return recv;
}

std::string ElliptecFlume::handleError(int errorCode)
{
	std::string errorMessage;
	switch (errorCode) {
	case 0:
		errorMessage = "OK, no error";
		break;
	case 1:
		errorMessage = "Communication time out";
		break;
	case 2:
		errorMessage = "Mechanical time out";
		break;
	case 3:
		errorMessage = "Command error or not supported";
		break;
	case 4:
		errorMessage = "Value out of range";
		break;
	case 5:
		errorMessage = "Module isolated";
		break;
	case 6:
		errorMessage = "Module out of isolation";
		break;
	case 7:
		errorMessage = "Initializing error";
		break;
	case 8:
		errorMessage = "Thermal error";
		break;
	case 9:
		errorMessage = "Busy";
		break;
	case 10:
		errorMessage = "Sensor Error (May appear during self-test. If code persists, there is an error)";
		break;
	case 11:
		errorMessage = "Motor Error (May appear during self-test. If code persists, there is an error)";
		break;
	case 12:
		errorMessage = "Out of Range (e.g., stage has been instructed to move beyond its travel range)";
		break;
	case 13:
		errorMessage = "Over Current error";
		break;
	default:
		errorMessage = "Unknown error code";
		break;
	}
	return errorMessage;  // Return the error message variable
}

void ElliptecFlume::readCallback(int byte)
{
	if (byte < 0 || byte >255) {
		thrower("Byte value readed needs to be in range 0-255.");
	}
	readRegister.push_back(byte);
	if (byte == '\n') {
		readComplete = true;
	}
}

void ElliptecFlume::errorCallback(std::string error)
{
	errorMsg = error;
}