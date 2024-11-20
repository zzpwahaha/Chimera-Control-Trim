#pragma once
#include <GeneralObjects/IChimeraSystem.h>
#include <PicoScrew/PicoScrewCore.h>
#include <ParameterSystem/ParameterSystemStructures.h>
#include <qcheckbox.h>
#include <qlabel.h>
#include <qlineedit.h>
#include <qpushbutton.h>

class IChimeraQtWindow;

class PicoScrewSystem : public IChimeraSystem
{
public:
	// THIS CLASS IS NOT COPYABLE.
	PicoScrewSystem& operator=(const PicoScrewSystem&) = delete;
	PicoScrewSystem(const PicoScrewSystem&) = delete;

	PicoScrewSystem(IChimeraQtWindow* parent);
	void initialize();
	void handleOpenConfig(ConfigStream& configFile);
	void handleSaveConfig(ConfigStream& configFile);
	void updateCurrentValues();
	void updateCurrentEditValue(unsigned channel, int pos); // used in external control
	void updateCtrlEnable();
	void handleProgramNowPress(std::vector<parameterType> constants);
	std::string getConfigDelim();
	PicoScrewCore& getCore() { return core; };
	std::string getDeviceInfo();

private:
	bool expActive;
	PicoScrewCore core;
	QCheckBox* ctrlButton;
	std::array<QLabel*, PICOSCREW_NUM> labels;
	std::array<QPushButton*, PICOSCREW_NUM> setHomeButtons;
	std::array<QLabel*, PICOSCREW_NUM> currentVals;
	std::array<QLineEdit*, PICOSCREW_NUM> edits;

};

