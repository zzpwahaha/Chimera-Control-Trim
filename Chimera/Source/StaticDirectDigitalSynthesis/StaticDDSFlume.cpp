#include "stdafx.h"
#include "StaticDDSFlume.h"

StaticDDSFlume::StaticDDSFlume(std::string portAddress, unsigned baudrate, bool safemode)
	: boostFlume(safemode, portAddress, baudrate)
	, SAFEMODE(safemode)
	, readComplete(true)
{
	boostFlume.setReadCallback(boost::bind(&StaticDDSFlume::readCallback, this, _1));
	boostFlume.setErrorCallback(boost::bind(&StaticDDSFlume::errorCallback, this, _1));
}

std::string StaticDDSFlume::query(std::string msg)
{
    return std::string();
}

void StaticDDSFlume::write(std::string msg)
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

// This should be called with a expectation of reading something, e.g. after writing and that is why the reading register etc is not initialized. 
// Otherwise it will throw
std::string StaticDDSFlume::read()
{
	if (SAFEMODE) {
		return std::string("static DDS is in safemode.");
	}
	std::string recv;
	/*read register after write*/
	for (auto idx : range(200)) {
		if (readComplete) {
			recv = std::string(readRegister.cbegin(), readRegister.cend());
			break;
		}
		Sleep(1);
	}
	/*check reading error and reading result and if reading is complete*/
	if (recv.empty() || !readComplete) {
		thrower("Reading is empty and timed out for 200ms in reading from windfreak serial port " + str(boostFlume.portID) + ".");
	}
	if (!errorMsg.empty()) {
		thrower("Nothing feeded back from static DDS, something might be wrong with it." + recv + "\r\nError message: " + errorMsg);
	}
	recv.erase(std::remove(recv.begin(), recv.end(), '\n'), recv.end());
	return recv;
}

void StaticDDSFlume::resetConnection()
{
	boostFlume.disconnect();
	Sleep(10);
	boostFlume.reconnect();
}

void StaticDDSFlume::readCallback(int byte)
{
	if (byte < 0 || byte >255) {
		thrower("Byte value readed needs to be in range 0-255.");
	}
	readRegister.push_back(byte);
	if (byte == '\n') {
		readComplete = true;
	}
}

void StaticDDSFlume::errorCallback(std::string error)
{
	errorMsg = error;
}
