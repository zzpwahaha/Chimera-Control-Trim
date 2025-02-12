#pragma once

#include <QMainWindow>
#include <QTimer>
#include "ConfigurationSystems/ProfileIndicator.h"
#include "ConfigurationSystems/profileSettings.h"
#include "ExperimentThread/ExperimentThreadInput.h"
#include "IChimeraQtWindow.h"
#include "ConfigurationSystems/ConfigSystem.h"
#include "MiscellaneousExperimentOptions/DebugOptionsControl.h"
#include "MiscellaneousExperimentOptions/MainOptionsControl.h"
#include "ExperimentMonitoringAndStatus/StatusControl.h"
#include "ExperimentMonitoringAndStatus/StatusIndicator.h"
#include "ExperimentMonitoringAndStatus/TemperatureMonitor.h"
#include "ConfigurationSystems/MasterConfiguration.h"
#include "MiscellaneousExperimentOptions/Repetitions.h"
#include "DataLogging/DataLogger.h"
#include "ConfigurationSystems/NoteSystem.h"
#include "ConfigurationSystems/profileSettings.h"
#include <ExternalController/TCPServerSystem.h>
#include <ExternalController/CommandModulator.h>
#include "GeneralUtilityFunctions/commonFunctions.h"
#include "GeneralObjects/RunInfo.h"
#include <AnalogInput/calInfo.h>
#include <string>
#include <vector>
#include <future>

namespace Ui {
    class QtMainWindow;
} 

class QtMainWindow : public IChimeraQtWindow{
    Q_OBJECT
		 
    public:
		explicit QtMainWindow ();
		~QtMainWindow () {};
		bool expIsRunning ();
		void initializeWidgets ();
		void showHardwareStatus ();

		void fillExpDeviceList (DeviceList& list);

		void onAutoCalFin (QString msg, profileSettings finishedConfig);
		void setStyleSheets ();
		//
		void loadCameraCalSettings (ExperimentThreadInput* input);
		void handlePauseToggle ();
		DeviceList getDevices ();
		// auxiliary functions used by the window.
		void setNotes (std::string notes);
		void fillMasterThreadInput (ExperimentThreadInput* input) override;
		void fillMotInput (ExperimentThreadInput* input);

		void startExperimentThread (ExperimentThreadInput* input);
				
		std::string getNotes ();
		profileSettings getProfileSettings ();
		debugInfo getDebuggingOptions ();
		mainOptions getMainOptions ();

		void checkProfileSave ();
		void updateConfigurationSavedStatus (bool status);

		void setDebuggingOptions (debugInfo options);
		void addTimebar (std::string whichStatus);
		void setShortStatus (std::string text);
		void changeShortStatusColor (std::string color);
		void changeBoxColor (std::string sysDelim, std::string color);
		void windowSaveConfig (ConfigStream& saveFile);
		void windowOpenConfig (ConfigStream& configFile);
		void abortMasterThread ();
		std::string getSystemStatusString ();
		bool masterIsRunning ();
		RunInfo getRunInfo();
		void handleFinishText ();
		unsigned getRepNumber ();
		void logParams (DataLogger* logger, ExperimentThreadInput* input);
		bool experimentIsPaused ();
		void notifyConfigUpdate ();

		void handleMasterConfigSave (std::stringstream& configStream);
		void handleMasterConfigOpen (ConfigStream& configStream);
		bool autoF5_AfterFinish = false;
		unsigned getAutoCalNumber ();

		QThread* getExpThread();
		ExpThreadWorker* getExpThreadWorker();
	public Q_SLOTS:
		void pauseExperiment();
		void handleColorboxUpdate (QString color, QString systemDelim);
		void handleNotification (QString txt, unsigned level=0);
		void onRepProgress (unsigned int repNum);
		void onErrorMessage (QString errMessage, unsigned level=0);
		void onNormalFinish (QString finMsg, profileSettings finishedProfile );
		void onFatalError (QString finMsg);

    private:
        Ui::QtMainWindow* ui;

		chronoTimes startupTimes;
		chronoTime* programStartTime;
		// members that have gui elements
		ConfigSystem profile;
		MasterConfiguration masterConfig;
		NoteSystem notes;
		TCPServerSystem tcpServer;
		DebugOptionsControl debugger;
		Repetitions repetitionControl;
		MainOptionsControl mainOptsCtrl;
		StatusControl mainStatus;
		StatusControl errorStatus;
		StatusIndicator shortStatus;

		TemperatureMonitor tempMonitor;

		ExpThreadWorker* expWorker;
		QThread* expThread = nullptr;
		std::atomic<bool> experimentIsRunning = false;

		RunInfo systemRunningInfo;
		//
		friend void commonFunctions::handleCommonMessage (int msgID, IChimeraQtWindow* win);
		friend class CommandModulator;
		unsigned autoCalNum = 0;

};

