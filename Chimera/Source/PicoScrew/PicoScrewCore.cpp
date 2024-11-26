#include "stdafx.h"
#include "PicoScrewCore.h"
#include <ConfigurationSystems/ConfigSystem.h>
#include <DataLogging/DataLogger.h>

PicoScrewCore::PicoScrewCore(bool safemode, std::string deviceKey)
	:safemode(safemode), 
	deviceKey(deviceKey), 
	screw(safemode, deviceKey)
{
	if (safemode) {
		return;
	}
	screw.write("MC"); // Check all motors to determine type
	for (auto channel : range(PICOSCREW_NUM)) {
		auto strChan = str(channel + 1);
		auto motorType = screw.query(strChan +"QM?"); // Motor type query
		if (motorType != "0" && motorType != "3") {
			thrower("Wrong motor type, should be either unconnected or standard motor, but get instead type " + motorType);
		}
		if (motorType == "0" && PICOSCREW_CONNECTED[channel]) {
			thrower("Error in detecting PICOSCEW number " + str(channel) + 
				" . It is supposed to be connected according to constant.h, but is NOT according to controller");
		}
		// hardcode the accerleration and velocity for all screws
		screw.write(strChan + "AC100000");
		screw.write(strChan + "VA750"); //medium velocity for minimizing hysterisis
	}
}

void PicoScrewCore::loadExpSettings(ConfigStream& stream)
{
	ConfigSystem::stdGetFromConfig(stream, *this, expSettings);
	experimentActive = expSettings.ctrlScrew;
}

void PicoScrewCore::logSettings(DataLogger& logger, ExpThreadWorker* threadworker)
{}

void PicoScrewCore::calculateVariations(std::vector<parameterType>& params, ExpThreadWorker* threadworker)
{
	if (!experimentActive && !expSettings.ctrlScrew) {
		return;
	}
	size_t totalVariations = (params.size() == 0) ? 1 : params.front().keyValues.size();
	try {
		for (auto ch : range(PICOSCREW_NUM)) {
			if (PICOSCREW_CONNECTED[ch]) {
				expSettings.screwPos[ch].assertValid(params, GLOBAL_PARAMETER_SCOPE);
				expSettings.screwPos[ch].internalEvaluate(params, totalVariations);
				if (expSettings.screwPos[ch].varies() && !PICOSCREW_CONNECTED[ch]) {
					thrower("Error in varying picoscrews for channel " + str(ch) +
						". This channel is NOT connected in constant.h but is varied given expression " +
						expSettings.screwPos[ch].expressionStr);
				}
			}
		}
	}
	catch (ChimeraError&) {
		throwNested("Failed to evaluate picoscrew expression varations!");
	}
}

void PicoScrewCore::programVariation(unsigned variation, std::vector<parameterType>& params, ExpThreadWorker* threadworker)
{
	if (!experimentActive && !expSettings.ctrlScrew) {
		return;
	}
	for (auto ch : range(PICOSCREW_NUM)) {
		if (PICOSCREW_CONNECTED[ch]) {
			moveTo(ch, expSettings.screwPos[ch].getValue(variation));
			for (auto idx : range(2000)) {
				if (motionDone(ch)) {
					int pos = motorPosition(ch);
					emit currentPosition(ch, pos);
					break;
				}
				Sleep(5); // move ~ 10 steps
				if (idx == 999) {
					thrower("Timed out in programming picoscrew channel " + str(ch) + "after 10 second. "
						"And the controller still shows motion-in-progress. Probably you shouldn't move motor this large");
				}
			}
		}
	}
}

picoScrewSettings PicoScrewCore::getSettingsFromConfig(ConfigStream& file)
{
	picoScrewSettings tempSettings;
	auto getlineF = ConfigSystem::getGetlineFunc(file.ver);
	//file.get();
	for (auto ch : range(PICOSCREW_NUM)) {
		getlineF(file, tempSettings.screwPos[ch].expressionStr);
	}
	file >> tempSettings.ctrlScrew;
	file.get();
	return tempSettings;
}

void PicoScrewCore::setHomePosition(unsigned channel, int position)
{
	if (position > 2147483647 || position < int(-2147483648)) {
		thrower("Position value out of allowable range for PicoScrew. Set value is: " + str(position));
	}
	auto strChan = str(channel + 1);
	screw.write(strChan + "DH" + str(position));
}

void PicoScrewCore::moveTo(unsigned channel, int position)
{
	if (position > 2147483647 || position < int(-2147483648)) {
		thrower("Position value out of allowable range for PicoScrew. Set value is: " + str(position));
	}
	auto strChan = str(channel + 1);
	screw.write(strChan + "PA" + str(position));
}

bool PicoScrewCore::motionDone(unsigned channel)
{
	auto strChan = str(channel + 1);
	auto done = screw.query(strChan + "MD?");
	return done == "1";
}

int PicoScrewCore::motorPosition(unsigned channel)
{
	auto strChan = str(channel + 1);
	auto posStr = screw.query(strChan + "TP?");
	int pos = 0;
	try {
		pos = boost::lexical_cast<int> (posStr);
	}
	catch (boost::bad_lexical_cast&) {
		if (posStr == "SAFEMODE") {
			return -1;
		}
		throwNested("Error in getting motor position for channel " + str(channel));
	}
	return pos;
}

std::string PicoScrewCore::getDeviceInfo()
{
	return screw.query("*IDN?");
}

void PicoScrewCore::setScrewExpSetting(picoScrewSettings tmpSetting)
{
	expSettings = tmpSetting;
}

