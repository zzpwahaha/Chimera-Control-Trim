#pragma once

#include "ConfigurationSystems/ProfileIndicator.h"
#include "ConfigurationSystems/profileSettings.h"
#include "ExperimentThread/ExperimentThreadInput.h"
#include "IChimeraQtWindow.h"

#include "LowLevel/constants.h"

#include "DigitalOutput/DoSystem.h"
#include "AnalogOutput/AoSystem.h"
#include "AnalogInput/AiSystem.h"
#include "DirectDigitalSynthesis/DdsSystem.h"
#include "OffsetLock/OlSystem.h"
#include "Microwave/MicrowaveSystem.h"
#include "PicoScrew/PicoScrewSystem.h"

#include "ParameterSystem/ParameterSystem.h"
#include "Scripts/Script.h"
#include "GeneralObjects/RunInfo.h"
#include "MiscellaneousExperimentOptions/Repetitions.h"
#include "ConfigurationSystems/MasterConfiguration.h"
#include "GeneralObjects/commonTypes.h"
#include "ExperimentMonitoringAndStatus/StatusControl.h"

#include "RealTimeDataAnalysis/MachineOptimizer.h"
#include "ExperimentThread/ExperimentThreadInput.h"
#include "AnalogInput/CalibrationManager.h"

#include "ExcessDialogs/doChannelInfoDialog.h"
#include "ExcessDialogs/AoSettingsDialog.h"
#include "ExcessDialogs/DdsSettingsDialog.h"
#include "ExcessDialogs/OlSettingsDialog.h"
#include "ExcessDialogs/AiSettingsDialog.h"

#include <QMainWindow>
#include <QTimer>

namespace Ui {
    class QtAuxiliaryWindow;
}

class QtAuxiliaryWindow : public IChimeraQtWindow{
    Q_OBJECT

    public:
        explicit QtAuxiliaryWindow (QWidget* parent= nullptr);
        ~QtAuxiliaryWindow ();

		void initializeWidgets ();
		void handleNormalFin ();
		void updateOptimization (AllExperimentInput& input);
		void handleMasterConfigSave (std::stringstream& configStream); // TODO get rid of these two, no longer in use. ZZP 06/26/2021
		void handleMasterConfigOpen (ConfigStream& configStream);
		/// Message Map Functions
		void ViewOrChangeTTLNames();
		void ViewOrChangeDACNames();
		void ViewOrChangeDDSNames();
		void ViewOrChangeOLNames();
		void ViewOrChangeAINames();
		void passRoundToDac ();
		std::string getOtherSystemStatusMsg ();

		std::array<std::string, size_t(DOGrid::total)> getTtlNames ();
		std::array<AoInfo, size_t(AOGrid::total)> getDacInfo ();
		std::array<std::string, size_t(AOGrid::total)> getDacNames();
		std::array<std::string, size_t(DDSGrid::total)> getDdsNames();
		std::array<std::string, size_t(OLGrid::total)> getOlNames();
		std::vector<std::string> getCalNames();

		DoSystem& getTtlSystem();
		DoCore& getTtlCore();
		AoSystem& getAoSys();
		DdsSystem& getDdsSys();
		OlSystem& getOlSys();
		PicoScrewSystem& getPsSys();
		AiSystem& getAiSys();
		CalibrationManager& getCalibManager();

		std::string getVisaDeviceStatus ();

		void fillMasterThreadInput(ExperimentThreadInput* input) override;
		void SetDacs ();
		void SetDds();
		void SetOls();

		void handleAbort ();
		void zeroDacs ();
		void zeroDds();
		void zeroOls();

		void relockPLL();
	 
		std::vector<parameterType> getAllParams ();
		std::vector<parameterType> getConfigParams();
		std::vector<parameterType> getGlobalParams();

		void clearVariables ();

		unsigned getTotalVariationNumber ();
		void windowSaveConfig (ConfigStream& saveFile);
		void windowOpenConfig (ConfigStream& configFile);
		std::pair<unsigned, unsigned> getTtlBoardSize ();
		unsigned getNumberOfDacs ();
		void setVariablesActiveState (bool active);



		ParameterSystem& getGlobals();
		ParameterSystem& getConfigs();
		std::vector<parameterType> getUsableConstants ();
		void fillExpDeviceList (DeviceList& list);
	protected:
		bool eventFilter (QObject* obj, QEvent* event) override;

    private:
        Ui::QtAuxiliaryWindow* ui;
		std::string title;
		/// control system classes
		DoSystem ttlBoard;
		AoSystem aoSys;
		AiSystem aiSys;
		DdsSystem dds;
		OlSystem olSys;
		MicrowaveSystem mwSys;
		PicoScrewSystem picoSys;

		MasterConfiguration masterConfig{ MASTER_CONFIGURATION_FILE_ADDRESS };
		MachineOptimizer optimizer;
		ParameterSystem configParamCtrl, globalParamCtrl;
		CalibrationManager calManager;

		doChannelInfoDialog* DOdialog;
		AoSettingsDialog* AOdialog;
		DdsSettingsDialog* DDSdialog;
		OlSettingsDialog* OLdialog;
		AiSettingsDialog* AIdialog;

		//std::vector<PlotCtrl*> aoPlots;
		//std::vector<PlotCtrl*> ttlPlots;
		unsigned NUM_DAC_PLTS = 3;
		unsigned NUM_TTL_PLTS = 4;

	public Q_SLOTS:
		//void handleDoAoPlotData (const std::vector<std::vector<plotDataVec>>& doData,
		//					     const std::vector<std::vector<plotDataVec>>& aoData);
		void updateExpActiveInfo (std::vector<parameterType> expParams);
		void updateCalActiveInfo (std::vector<calSettings> expCalParams);
};

