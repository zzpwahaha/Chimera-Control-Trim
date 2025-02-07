// created by Mark O. Brown
#pragma once

#include "ConfigurationSystems/Version.h"
#include "GeneralImaging/softwareAccumulationOption.h"
#include "ConfigurationSystems/ConfigStream.h"
#include "Andor/andorPicSettingsGroup.h"
#include <array>
#include <vector>
#include "PrimaryWindows/IChimeraQtWindow.h"
#include <qlabel.h>
#include <CustomQtControls/AutoNotifyCtrls.h>

class AndorCameraCore;
class AndorCameraSettingsControl;

struct displayTypeOption{
	bool isDiff = false;
	// zero-indexed.
	unsigned whichPicForDif = 0;
};

/*
 * This class handles all of the gui objects for assigning camera settings. It works closely with the Andor class
 * because it eventually needs to communicate all of these settings to the Andor class.
 */
class PictureSettingsControl : public QWidget
{
	Q_OBJECT
	public:
		// must have parent. Enforced partially because both are singletons.
		PictureSettingsControl();
		void updateAllSettings ( andorPicSettingsGroup inputSettings );
		void handleSaveConfig(ConfigStream& saveFile);
		void handleOpenConfig(ConfigStream& openFile, AndorCameraCore* andor);
		void initialize( IChimeraQtWindow* parent );
		void handleOptionChange( );
		void setPictureControlEnabled (int pic, bool enabled);
		void setUnofficialExposures ( std::vector<float> times );
		std::array<int, 4> getPictureColors ( );
		std::array<float, 4> getExposureTimes ( );
		std::vector<float> getUsedExposureTimes();
		std::array<std::vector<int>, 4> getThresholds();
		std::array<displayTypeOption, 4> getDisplayTypeOptions( );
		void setThresholds( std::array<std::string, 4> thresholds);
		bool getContinuousMode();
		unsigned getPicsPerRepetition();
		unsigned getPicsPerRepetitionNonContinuous();
		void updateSettings( );
		void updateColormaps ( std::array<int, 4> colorsIndexes );
		void setContinuousMode(bool contMode, unsigned picNum);
		void setUnofficialPicsPerRep( unsigned picNum);
		std::array<std::string, 4> getThresholdStrings();
		std::array<softwareAccumulationOption, 4> getSoftwareAccumulationOptions ( );
		void setSoftwareAccumulationOptions ( std::array<softwareAccumulationOption, 4> opts );
		static andorPicSettingsGroup getPictureSettingsFromConfig (ConfigStream& configFile );
		void setEnabledStatus (bool viewRunningSettings);

		void toggleExposureTimeEditGui(bool enable);
	private:
		// the internal memory of the settings here is somewhat redundant with the gui objects. It'd probably be better
		// if this didn't exist and all the getters just converted straight from the gui objects, but that's a 
		// refactoring for another time.
		andorPicSettingsGroup currentPicSettings;
		unsigned unofficialPicsPerRep=1;
		/// Grid of PictureOptions
		CQCheckBox* continuousModeChk;
		QSpinBox* picsPerRepEdit;
		QLabel* totalPicNumberLabel;
		QLabel* pictureLabel;
		QLabel* exposureLabel;
		QLabel* thresholdLabel;
		QLabel* colormapLabel;
		QLabel* displayTypeLabel;
		QLabel* softwareAccumulationLabel;
		//QLabel* picScaleFactorLabel;
		//QLineEdit* picScaleFactorEdit;
		//QLabel* transfModeLabel;
		//CQComboBox* transformationModeCombo;
		// 
		std::array<CQRadioButton*, 4> totalNumberChoice = std::array<CQRadioButton*, 4>();
		std::array<QLabel*, 4> pictureNumbers = std::array<QLabel*, 4>();
		std::array<CQLineEdit*, 4> exposureEdits = std::array<CQLineEdit*, 4>();
		std::array<CQLineEdit*, 4> thresholdEdits = std::array<CQLineEdit*, 4>();
		std::array<CQComboBox*, 4> colormapCombos = std::array<CQComboBox*, 4>();
		std::array<CQComboBox*, 4> displayTypeCombos = std::array<CQComboBox*, 4>();
		std::array<CQCheckBox*, 4> softwareAccumulateAll = std::array<CQCheckBox*, 4>();
		std::array<CQLineEdit*, 4> softwareAccumulateNum = std::array<CQLineEdit*, 4>();
};


