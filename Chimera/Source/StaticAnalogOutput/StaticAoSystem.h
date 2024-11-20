#pragma once
#include <GeneralObjects/IChimeraSystem.h>
#include <ParameterSystem/ParameterSystem.h>
#include <StaticAnalogOutput/StaticAoCore.h>

class IChimeraQtWindow;
class StaticAoSystem : public IChimeraSystem
{
	Q_OBJECT
public:
	// THIS CLASS IS NOT COPYABLE.
	StaticAoSystem& operator=(const StaticAoSystem&) = delete;
	StaticAoSystem(const StaticAoSystem&) = delete;
	StaticAoSystem(IChimeraQtWindow* parent);

	void initialize();
	void handleOpenConfig(ConfigStream& configFile);
	void handleSaveConfig(ConfigStream& configFile);
	void updateCtrlEnable();
	void handleProgramNowPress(std::vector<parameterType> constants);
	std::string getConfigDelim() { return core.getDelim(); };
	StaticAoCore& getCore() { return core; };
	std::string getDeviceInfo();

private:
	bool expActive;
	StaticAoCore core;
	QCheckBox* ctrlButton;
	std::array<QLabel*, size_t(StaticAOGrid::total)> labels;
	std::array<QLineEdit*, size_t(StaticAOGrid::total)> edits;


};

