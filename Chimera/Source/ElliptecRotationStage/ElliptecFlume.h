#pragma once
#include <GeneralFlumes/BoostAsyncSerial.h>

class ElliptecFlume
{
public:
	ElliptecFlume(std::string portAddress, bool safemode);
	std::string query(std::string msg);
	std::string queryWithCheck(std::string msg);
	void write(std::string msg);
	std::string readWithCheck();
	std::string queryIdentity(unsigned identifier);
	void resetConnection();
	const bool SAFEMODE;
private:
	std::string read();
	std::string handleError(int errorCode);

	void readCallback(int byte);
	void errorCallback(std::string error);
	BoostAsyncSerial boostFlume;
	std::atomic<bool> readComplete;
	std::vector<unsigned char> readRegister;
	std::string errorMsg;
};

