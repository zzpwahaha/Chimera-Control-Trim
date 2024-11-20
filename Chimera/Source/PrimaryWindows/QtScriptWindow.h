#pragma once

#include <QMainWindow>
#include <QTimer>
#include "ConfigurationSystems/ProfileIndicator.h"
#include "Scripts/Script.h"
#include "ArbGen/ArbGenSystem.h"
#include <ArbGen/whichAg.h>
#include <GigaMOOG/GigaMoogSystem.h>
#include "ConfigurationSystems/profileSettings.h"
#include "ExperimentThread/ExperimentThreadInput.h"
#include "IChimeraQtWindow.h"
 
// a convenient structure for containing one object for each script. For example, the address of each script.
template <typename type> struct scriptInfo{
	type master;
	type gmoog;
};

namespace Ui {
    class QtScriptWindow;
}

class QtScriptWindow : public IChimeraQtWindow{
    Q_OBJECT

    public:
        explicit QtScriptWindow (QWidget* parent= nullptr);
        ~QtScriptWindow ();
		void initializeWidgets ();

		void fillExpDeviceList (DeviceList& list);
		void fillMasterThreadInput(ExperimentThreadInput* input) override {};
		void checkScriptSaves ();

		scriptInfo<std::string> getScriptNames ();
		scriptInfo<bool> getScriptSavedStatuses ();
		scriptInfo<std::string> getScriptAddresses ();

		profileSettings getProfileSettings ();
		profileSettings getProfile();

		std::string getSystemStatusString ();
		void updateDoAoDdsNames ();
		void checkMasterSave ();
		
		void windowSaveConfig (ConfigStream& saveFile);
		void windowOpenConfig (ConfigStream& configFile);

		//void updateProfile (std::string text);
		void considerScriptLocations();

		void updateArbGen(ArbGenEnum::name name);
		void newArbGenScript(ArbGenEnum::name name);
		void openArbGenScript(ArbGenEnum::name name, IChimeraQtWindow* parent);
		void saveArbGenScript(ArbGenEnum::name name);
		void saveArbGenScriptAs(ArbGenEnum::name name, IChimeraQtWindow* parent);


		void newMasterScript ();
		void openMasterScript (IChimeraQtWindow* parent);
		void openMasterScript (std::string name, bool askMove = true);
		void saveMasterScript ();
		void saveMasterScriptAs (IChimeraQtWindow* parent);
		void newMasterFunction ();
		void reloadMasterFunction();
		void saveMasterFunction ();
		void deleteMasterFunction ();

		void newGMoogScript();
		void openGMoogScript(IChimeraQtWindow* parent);
		void openGMoogScript(std::string name);
		void saveGMoogScript();
		void saveGMoogScriptAs(IChimeraQtWindow* parent);

		void saveAllScript();

		void updateConfigurationSavedStatus (bool status);

		/*these two seems not in use -zzp2021/02/25*/
		void handleMasterFunctionChange ();
		//void handleIntensityCombo();

		/* for normal/abort finish add it later, QtMainWindow::onNormalFinish,QtMainWindow::onFatalError -zzp 20210225*/
		void setIntensityDefault();

		std::vector<std::reference_wrapper<ArbGenSystem>> getArbGenSystem();
		std::vector<std::reference_wrapper<ArbGenCore>> getArbGenCore();

    private:
        Ui::QtScriptWindow* ui;
        Script masterScript;
		//Script gmoogScript;
        //ProfileIndicator profileDisplay;

		std::array<ArbGenSystem, numArbGen> arbGens;
		GigaMoogSystem gigaMoog;

	public Q_SLOTS:
		void updateVarNames ();
};

