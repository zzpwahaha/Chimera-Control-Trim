#pragma once
#include <string>
#include <GeneralFlumes/BoostAsyncSerial.h>

class StaticDDSFlume {
public:
	StaticDDSFlume(std::string portAddress, unsigned baudrate, bool safemode);
	std::string query(std::string msg);
	void write(std::string msg);
	std::string read();
	void resetConnection();
	const bool SAFEMODE;
private:
	void readCallback(int byte);
	void errorCallback(std::string error);
	BoostAsyncSerial boostFlume;
	std::atomic<bool> readComplete;
	std::vector<unsigned char> readRegister;
	std::string errorMsg;

};