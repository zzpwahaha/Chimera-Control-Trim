#pragma once

#include "GeneralObjects/commonTypes.h"
#include "ConfigurationSystems/ConfigStream.h"
#include "ConfigurationSystems/Version.h"
#include "ExperimentThread/DeviceList.h"
#include <ExperimentThread/ExperimentThreadInput.h>
#include <array>
#include <QMainWindow>


class ColorBox;
class QtMainWindow;
class QtScriptWindow;
class QtAuxiliaryWindow;
class QtAndorWindow;
class QtMakoWindow;
class QtAnalysisWindow;

namespace Ui {
	class IChimeraQtWindow;
}

class IChimeraQtWindow : public QMainWindow{
	Q_OBJECT
	public:
		explicit IChimeraQtWindow (QWidget* parent = nullptr);
		~IChimeraQtWindow () {};
		// any chimera window should override these functions in order to handle
		virtual void windowOpenConfig (ConfigStream& configFile)=0;
		virtual void windowSaveConfig (ConfigStream& newFile)=0;
		// Basically every window has devices which should be programmed on it, 
		// so every window must have a function to add them.
		virtual void fillExpDeviceList (DeviceList& list) = 0;
		virtual void initializeWidgets () = 0;
		virtual void fillMasterThreadInput(ExperimentThreadInput* input) = 0;
		void initializeShortcuts ();
		void initializeMenu ();
		void loadFriends (QtMainWindow* mainWin_, QtScriptWindow* scriptWin_, QtAuxiliaryWindow* auxWin_,
						  QtAndorWindow* andorWin_, QtMakoWindow* makoWin1_, QtMakoWindow* makoWin2_, QtAnalysisWindow* analysisWin_);
		void reportErr (QString errStr, unsigned errorLevel=0);
		void reportStatus (QString statusStr, unsigned notificationLevel=0);

		std::vector<IChimeraQtWindow*> winList();
		QtMainWindow* mainWin = nullptr;
		QtScriptWindow* scriptWin = nullptr;
		QtAndorWindow* andorWin = nullptr;
		QtAuxiliaryWindow* auxWin = nullptr;
		QtMakoWindow* makoWin1 = nullptr;
		QtMakoWindow* makoWin2 = nullptr;
		QtAnalysisWindow* analysisWin = nullptr;
		static constexpr unsigned numWindows = 6;
		ColorBox* statBox;
		virtual void changeBoxColor (std::string sysDelim, std::string color);
	private:
		Ui::IChimeraQtWindow* ui;
	public Q_SLOTS:
		void configUpdated ();
};
