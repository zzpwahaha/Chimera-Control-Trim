#pragma once
#include <GeneralObjects/IChimeraSystem.h>
#include <ParameterSystem/ParameterSystem.h>
#include <ElliptecRotationStage/ElliptecCore.h>
#include <qcheckbox.h>
#include <qlabel.h>
#include <qlineedit.h>
#include <qpushbutton.h>

class ElliptecSystem : public IChimeraSystem
{
	Q_OBJECT
public:
	// THIS CLASS IS NOT COPYABLE.
	ElliptecSystem& operator=(const ElliptecSystem&) = delete;
	ElliptecSystem(const ElliptecSystem&) = delete;
	ElliptecSystem(IChimeraQtWindow* parent);

	void initialize();
	void handleOpenConfig(ConfigStream& configFile);
	void handleSaveConfig(ConfigStream& configFile);
	void updateCtrlEnable();
	void handleProgramNowPress(std::vector<parameterType> constants);
	std::string getConfigDelim() { return core.getDelim(); };
	ElliptecCore& getCore() { return core; };
	std::string getDeviceInfo();
	void updateCurrentValues();

private:
	bool expActive;
	ElliptecCore core;
	QCheckBox* ctrlButton;
	std::array<QLabel*, size_t(ElliptecGrid::total)> labels;
	//std::array<QLabel*, size_t(ElliptecGrid::total)> currentVals;
	std::array<QLineEdit*, size_t(ElliptecGrid::total)> edits;
};

