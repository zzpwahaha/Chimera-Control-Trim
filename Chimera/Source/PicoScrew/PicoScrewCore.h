#pragma once
#include <GeneralObjects/IDeviceCore.h>
#include <PicoScrew/PicoScrewFlume.h>
#include <ParameterSystem/Expression.h>
#include <ConfigurationSystems/ConfigStream.h>

class PicoScrewCore;

struct picoScrewSettings
{
	std::array<Expression, PICOSCREW_NUM> screwPos;
	bool ctrlScrew;
};

class PicoScrewCore : public IDeviceCore
{
	Q_OBJECT
public:
	// THIS CLASS IS NOT COPYABLE.
	PicoScrewCore& operator=(const PicoScrewCore&) = delete;
	PicoScrewCore(const PicoScrewCore&) = delete;
	PicoScrewCore(bool safemode, std::string deviceKey);

	void loadExpSettings(ConfigStream& stream) override;
	void logSettings(DataLogger& logger, ExpThreadWorker* threadworker) override;
	void calculateVariations(std::vector<parameterType>& params, ExpThreadWorker* threadworker) override;
	void programVariation(unsigned variation, std::vector<parameterType>& params,
		ExpThreadWorker* threadworker) override;
	void normalFinish() override {};
	void errorFinish() override {};
	std::string getDelim() override { return configDelim; };

	picoScrewSettings getSettingsFromConfig(ConfigStream& file);

	void setHomePosition(unsigned channel, int position = 0);
	void moveTo(unsigned channel, int position);
	bool motionDone(unsigned channel);
	int motorPosition(unsigned channel);
	std::string getDeviceInfo();
	void setScrewExpSetting(picoScrewSettings tmpSetting); // used only for ProgramNow in PicoScrewSystem

	const bool safemode;
	const std::string configDelim = "PICOSCREW";
	const std::string deviceKey;

signals:
	void currentPosition(unsigned id, int pos);

private:
	PicoScrewFlume screw;
	picoScrewSettings expSettings;
};

