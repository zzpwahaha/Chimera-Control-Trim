// created by Mark O. Brown

#pragma once

#include "ConfigurationSystems/Version.h"
#include "ConfigurationSystems/ConfigStream.h"
#include <unordered_map>
#include <QLabel>
#include <PrimaryWindows/IChimeraQtWindow.h>
#include <CustomQtControls/AutoNotifyCtrls.h>

class Repetitions :public QWidget
{
	Q_OBJECT
	public:
		void initialize(IChimeraQtWindow* mainWin );
		void setRepetitions(unsigned number);
		unsigned int getRepetitionNumber();
		static unsigned getSettingsFromConfig (ConfigStream& openFile );
		void updateNumber(long repNumber);
		void handleSaveConfig(ConfigStream& saveFile);
	private:
		unsigned repetitionNumber;
		CQLineEdit* repetitionEdit;
		QLabel* repetitionDisp;
		QLabel* repetitionText;
};
