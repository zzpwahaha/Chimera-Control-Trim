#include "stdafx.h"
#include "StaticAoCore.h"
#include <ExperimentThread/ExpThreadWorker.h>
#include <ConfigurationSystems/ConfigSystem.h>
#include <DataLogging/DataLogger.h>



StaticAoCore::StaticAoCore(bool safemode, const std::string& host, int port)
	: safemode(safemode)
	, fpga(safemode, host, port)
{
}

void StaticAoCore::loadExpSettings(ConfigStream& stream)
{
	ConfigSystem::stdGetFromConfig(stream, *this, expSettings);
	experimentActive = expSettings.ctrlAO;
}

void StaticAoCore::logSettings(DataLogger& logger, ExpThreadWorker* threadworker)
{}

void StaticAoCore::calculateVariations(std::vector<parameterType>& params, ExpThreadWorker* threadworker)
{
	if (!experimentActive && !expSettings.ctrlAO) {
		return;
	}
	size_t totalVariations = (params.size() == 0) ? 1 : params.front().keyValues.size();
	try {
		for (auto ch : range(size_t(StaticAOGrid::total))) {
			expSettings.staticAOs[ch].assertValid(params, GLOBAL_PARAMETER_SCOPE);
			expSettings.staticAOs[ch].internalEvaluate(params, totalVariations);
			if (expSettings.staticAOs[ch].varies() && safemode) {
				thrower("Error in varying 20-bit dac for channel " + str(ch) +
					". The DAC is in SAFEMODE in constant.h but is varied given expression " +
					expSettings.staticAOs[ch].expressionStr);
			}
			for (auto variation : range(totalVariations)) {
				auto dacVal = expSettings.staticAOs[ch].getValue(variation);
				if (!checkBound(dacVal)) {
					thrower("Error in varying 20-bit dac for channel " + str(ch) + " and variation" + str(variation) + 
						". The DAC is limited to " + str(minVal) + " V to " + str(maxVal) + 
						" V but is set to an outside value given expression " +
						expSettings.staticAOs[ch].expressionStr + " and its evaluation: " + str(dacVal));
				}
			}
		}
	}
	catch (ChimeraError&) {
		throwNested("Failed to evaluate staticAO expression varations!");
	}
}

void StaticAoCore::programVariation(unsigned variation, std::vector<parameterType>& params, ExpThreadWorker* threadworker)
{
	if (!experimentActive && !expSettings.ctrlAO) {
		return;
	}
	std::array<double, size_t(StaticAOGrid::total)> outputs;
	for (auto ch : range(size_t(StaticAOGrid::total))) {
		outputs[ch] = expSettings.staticAOs[ch].getValue(variation);
	}
	writeDACs(outputs);
}

StaticAOSettings StaticAoCore::getSettingsFromConfig(ConfigStream& file)
{
	StaticAOSettings tempSettings;
	auto getlineF = ConfigSystem::getGetlineFunc(file.ver);
	//file.get();
	for (auto ch : range(size_t(StaticAOGrid::total))) {
		getlineF(file, tempSettings.staticAOs[ch].expressionStr);
	}
	file >> tempSettings.ctrlAO;
	file.get();
	return tempSettings;
}

std::string StaticAoCore::getDeviceInfo()
{
	return std::string("20 bit DAC - AD5791");
}

void StaticAoCore::setStaticAOExpSetting(StaticAOSettings tmpSetting)
{
	expSettings = tmpSetting;
}

std::vector<unsigned char> StaticAoCore::getDACbytes(double dacVal)
{
	// Normalize the voltage to the range 0 to 1
	float normalized_voltage = (dacVal + 10.0f) / 20.0f;
	unsigned long bit_value = static_cast<unsigned long>(normalized_voltage * (1 << 20) - 1);
	// Create a 24-bit value with a leading 4-bit 0x00
	unsigned long combined_value = bit_value & 0xFFFFF;  // Mask to get only the lower 20 bits
	std::vector<unsigned char> bytes(3);
	bytes[0] = (combined_value >> 16) & 0xFF;  // Highest 8 bits
	bytes[1] = (combined_value >> 8) & 0xFF;   // Middle 8 bits
	bytes[2] = combined_value & 0xFF;          // Lowest 8 bits
	return bytes;
}

void StaticAoCore::writeDACs(std::array<double, size_t(StaticAOGrid::total)> outputs)
{
	std::vector<unsigned char> command;
	command.reserve(outputs.size() * 3); // 3 bytes per channel
	for (auto it = outputs.rbegin(); it != outputs.rend(); ++it) {
		std::vector<unsigned char> dac_result = getDACbytes(*it);
		command.insert(command.end(), dac_result.begin(), dac_result.end());
	}
	fpga.write(command);
}

bool StaticAoCore::checkBound(double dacVal)
{
	if (dacVal > maxVal) {
		return false;
	}
	else if (dacVal < minVal) {
		return false;
	}
	else {
		return true;
	}
}