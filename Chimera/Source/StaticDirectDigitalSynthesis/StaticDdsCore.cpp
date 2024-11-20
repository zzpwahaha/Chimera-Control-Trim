#include "stdafx.h"
#include "StaticDdsCore.h"
#include <ExperimentThread/ExpThreadWorker.h>
#include <ConfigurationSystems/ConfigSystem.h>
#include <DataLogging/DataLogger.h>

StaticDdsCore::StaticDdsCore(bool safemode, const std::string& port, unsigned baudrate) :
	safemode(safemode), 
	sddsFlume(port, baudrate, safemode)
{
}

void StaticDdsCore::loadExpSettings(ConfigStream& stream)
{
	ConfigSystem::stdGetFromConfig(stream, *this, expSettings);
	experimentActive = expSettings.ctrlDDS;
}

void StaticDdsCore::logSettings(DataLogger& logger, ExpThreadWorker* threadworker)
{}

void StaticDdsCore::calculateVariations(std::vector<parameterType>&params, ExpThreadWorker * threadworker)
{
	if (!experimentActive && !expSettings.ctrlDDS) {
		return;
	}
	size_t totalVariations = (params.size() == 0) ? 1 : params.front().keyValues.size();
	try {
		for (auto ch : range(size_t(StaticDDSGrid::total))) {
			expSettings.staticDDSs[ch].assertValid(params, GLOBAL_PARAMETER_SCOPE);
			expSettings.staticDDSs[ch].internalEvaluate(params, totalVariations);
			if (expSettings.staticDDSs[ch].varies() && safemode) {
				thrower("Error in varying static DDS for channel " + str(ch) +
					". The DDS is in SAFEMODE in constant.h but is varied given expression " +
					expSettings.staticDDSs[ch].expressionStr);
			}
			for (auto variation : range(totalVariations)) {
				auto ddsfreqVal = expSettings.staticDDSs[ch].getValue(variation);
				if (!checkBound(ddsfreqVal)) {
					thrower("Error in varying static DDS for channel " + str(ch) + " and variation" + str(variation) +
						". The DDS is limited to " + str(minVal) + " MHz to " + str(maxVal) +
						" MHz but is set to an outside value given expression " +
						expSettings.staticDDSs[ch].expressionStr + " and its evaluation: " + str(ddsfreqVal));
				}
			}
		}
	}
	catch (ChimeraError&) {
		throwNested("Failed to evaluate staticAO expression varations!");
	}
}

void StaticDdsCore::programVariation(unsigned variation, std::vector<parameterType>& params, ExpThreadWorker* threadworker)
{
	if (!experimentActive && !expSettings.ctrlDDS) {
		return;
	}
	std::array<double, size_t(StaticDDSGrid::total)> outputs;
	for (auto ch : range(size_t(StaticDDSGrid::total))) {
		outputs[ch] = expSettings.staticDDSs[ch].getValue(variation);
	}
	writeDDSs(outputs);
}

StaticDDSSettings StaticDdsCore::getSettingsFromConfig(ConfigStream& file)
{
	StaticDDSSettings tempSettings;
	auto getlineF = ConfigSystem::getGetlineFunc(file.ver);
	//file.get();
	for (auto ch : range(size_t(StaticDDSGrid::total))) {
		getlineF(file, tempSettings.staticDDSs[ch].expressionStr);
	}
	file >> tempSettings.ctrlDDS;
	file.get();
	return tempSettings;
}

std::string StaticDdsCore::getDeviceInfo()
{
	return std::string("3.5-GSPS 12-bit DDS - AD9914");
}

void StaticDdsCore::setStaticDDSExpSetting(StaticDDSSettings tmpSetting)
{
	expSettings = tmpSetting;
}

std::string StaticDdsCore::getDDSCommand(double ddsfreqVal)
{
	std::string buffCmd;
	buffCmd += "(" + str(0/*getCmdChannelIdx(channelSnap.channel)*/) + ","
		+ str(ddsfreqVal/*channelSnap.val*/, numFreqDigits) + ","
		+ str(ddsfreqVal/*channelSnap.endVal*/, numFreqDigits) + "," + str(1/*channelSnap.numSteps*/) + ","
		+ str(1.0/*channelSnap.rampTime*/, 2/*numTimeDigits(channelSnap.channel)*/) + ")";
	buffCmd += "e";
	return buffCmd;
}

void StaticDdsCore::writeDDSs(std::array<double, size_t(StaticDDSGrid::total)> outputs)
{
	std::string command;
	for (auto ch : range(size_t(StaticDDSGrid::total))) {
		command = getDDSCommand(outputs[ch]);
	}
	sddsFlume.write(command);
	if (!safemode) {
		std::string recv = sddsFlume.read();
		std::transform(recv.begin(), recv.end(), recv.begin(), ::tolower); /*:: without namespace select from global namespce, see https://stackoverflow.com/questions/5539249/why-cant-transforms-begin-s-end-s-begin-tolower-be-complied-successfu*/
		if (recv.find("error") != std::string::npos) {
			thrower("Error in static DDS programming, from Arduino: " + recv);
		}
	}
}

bool StaticDdsCore::checkBound(double ddsfreqVal)
{
	if (ddsfreqVal > maxVal) {
		return false;
	}
	else if (ddsfreqVal < minVal) {
		return false;
	}
	else {
		return true;
	}
}
