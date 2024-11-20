#pragma once
// created by Mark O. Brown

#include <AnalogInput/calInfo.h>
#include "AnalogInput/AiSystem.h"
#include <AnalogInput/CalibrationThreadWorker.h>
#include "AnalogOutput/AoSystem.h"
#include "DigitalOutput/DoSystem.h"
#include "ParameterSystem/ParameterSystem.h"
#include "ConfigurationSystems/Version.h"
#include <CustomQtControls/AutoNotifyCtrls.h>
#include <ArbGen/ArbGenCore.h>
#include <Python/NewPythonHandler.h>
#include <type_traits>
#include <condition_variable>
#include <QLabel>
#include <QTableWidget>
#include <QPushButton>

class IChimeraQtWindow;
class ExpThreadWorker;
class ConfigStream;

class CalibrationManager : public IChimeraSystem {
	public:
		// THIS CLASS IS NOT COPYABLE.
		CalibrationManager& operator=(const CalibrationManager&) = delete;
		CalibrationManager (const CalibrationManager&) = delete;
		CalibrationManager (IChimeraQtWindow* parent);

		static void determineCalMinMax (calResult& res);
		void handleContextMenu (const QPoint& pos);
		std::string calTtlConfigToString (std::vector<std::pair<unsigned, unsigned> > ttlConfig);
		std::string calDacConfigToString (std::vector<std::pair<unsigned, double>> aoConfig);
		void initialize (IChimeraQtWindow* parent, AiSystem* ai, AoSystem* ao, DoSystem* ttls_in,
			std::vector<std::reference_wrapper<ArbGenCore>> arbGensIn, NewPythonHandler* python_in);
		void runAllThreaded ();
		void inExpRunAllThreaded(ExpThreadWorker* expThread, bool calibrateOnlyActive);
		void calibrateThreaded (calSettings& cal, unsigned which);
		bool wantsExpAutoCal ();
		void handleSaveConfig(ConfigStream& configStream);
		void handleSaveMasterConfig (ConfigStream& configStream);
		void handleSaveMasterConfigIndvResult (ConfigStream& configStream, calResult& cal);
		void handleOpenMasterConfigIndvResult (ConfigStream& configStream, calResult& result);
		void handleOpenMasterConfig (ConfigStream& configStream);
		void handleOpenConfig(ConfigStream& configStream);
		std::vector<calSettings> getCalibrationInfo ();
		void standardStartThread (std::vector<std::reference_wrapper<calSettings>> calibrations, ExpThreadWorker* expThread= nullptr);
		void setCalibrations(std::vector<calSettings> cals);
		void refreshListview ();
		static std::vector<double> calPtTextToVals (QString qtxt);
		// no boundary check is only used in setting the plot of the calibration, in the experiment, the boundary check should always be on
		static double calibrationFunction (double val, calResult& calibration, IChimeraSystem* parent = nullptr, bool boundaryCheck = true);

		std::mutex& calLock() { return calibrationLock; }
		std::condition_variable& calConditionVariable() { return calibrationConditionVariable; }
		bool isCalibrationRunning() { return calibrationRunning; }

	public:
		const std::string systemDelim = "CALIBRATION_MANAGER";

	private:
		bool calibrationRunning = false;
		QLabel* calsHeader;
		CQPushButton* calibrateAllButton;
		CQCheckBox* expAutoCalButton;
		QTableWidget* calibrationTable;
		QPushButton* cancelCalButton;
		QCustomPlotCtrl calibrationViewer;
		//PlotCtrl calibrationViewer;
		void updateCalibrationView (calSettings& cal);
		void handleSaveMasterConfigIndvCal (ConfigStream& configStream, calSettings& servo);
		calSettings handleOpenMasterConfigIndvCal (ConfigStream& configStream);
		std::vector<calSettings> calibrations;
		void addCalToListview (calSettings& cal);
		CalibrationThreadWorker* threadWorker;
		QThread* thread;
		AiSystem* ai;
		AoSystem* ao;
		DoSystem* ttls;
		std::vector<std::reference_wrapper<ArbGenCore>> arbGens;
		NewPythonHandler* pythonHandler;
		// for expThread in-exp calibration
		std::mutex calibrationLock;
		std::condition_variable calibrationConditionVariable;

		friend class CommandModulator;
};



