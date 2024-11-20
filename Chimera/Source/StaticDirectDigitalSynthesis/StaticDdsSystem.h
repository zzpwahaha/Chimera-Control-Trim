#pragma once
#include <GeneralObjects/IChimeraSystem.h>
#include <ParameterSystem/ParameterSystem.h>
#include <StaticDirectDigitalSynthesis/StaticDdsCore.h>

class IChimeraQtWindow;
class StaticDdsSystem : public IChimeraSystem
{
	Q_OBJECT
public:
	// THIS CLASS IS NOT COPYABLE.
	StaticDdsSystem& operator=(const StaticDdsSystem&) = delete;
	StaticDdsSystem(const StaticDdsSystem&) = delete;
	StaticDdsSystem(IChimeraQtWindow* parent);

	void initialize();
	void handleOpenConfig(ConfigStream& configFile);
	void handleSaveConfig(ConfigStream& configFile);
	void updateCtrlEnable();
	void handleProgramNowPress(std::vector<parameterType> constants);
	std::string getConfigDelim() { return core.getDelim(); };
	StaticDdsCore& getCore() { return core; };
	std::string getDeviceInfo();

	void setDdsEditValue(std::string ddsfreq, unsigned channel); // should only be used in CommandModulator
private:
	bool expActive;
	StaticDdsCore core;
	QCheckBox* ctrlButton;
	std::array<QLabel*, size_t(StaticDDSGrid::total)> labels;
	std::array<QLineEdit*, size_t(StaticDDSGrid::total)> edits;


};


