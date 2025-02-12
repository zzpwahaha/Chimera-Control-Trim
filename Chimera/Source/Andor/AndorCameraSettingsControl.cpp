// created by Mark O. Brown
#include "stdafx.h"

#include "AndorCameraSettingsControl.h"

#include "PrimaryWindows/QtAndorWindow.h"
#include "GeneralUtilityFunctions/miscCommonFunctions.h"
#include "ConfigurationSystems/ConfigSystem.h"

#include <boost/lexical_cast.hpp>

AndorCameraSettingsControl::AndorCameraSettingsControl() : imageDimensionsObj("andor"){
	AndorRunSettings& andorSettings = configSettings.andor;
}

void AndorCameraSettingsControl::initialize ( IChimeraQtWindow* parent, std::vector<std::string> vertSpeeds,
											  std::vector<std::string> horSpeeds )
{
	QVBoxLayout* layout = new QVBoxLayout(this);
	layout->setContentsMargins(0, 0, 0, 0);

	header = new QLabel ("ANDOR CAMERA SETTINGS", parent);
	programNow = new QPushButton ("Program Now", parent);
	parent->connect (programNow, &QPushButton::released, parent->andorWin, [parent, this]() {
			auto settings = getConfigSettings ().andor;
			parent->andorWin->handlePrepareForAcq (&settings, analysisSettings());
			parent->andorWin->manualArmCamera ();
		});
	layout->addWidget(header, 1);
	layout->addWidget(programNow, 1);

	QHBoxLayout* layout1 = new QHBoxLayout(this);
	layout1->setContentsMargins(0, 0, 0, 0);
	controlAndorCameraCheck = new CQCheckBox ("Control Andor Camera?", parent);
	controlAndorCameraCheck->setChecked (true);
	viewRunningSettings = new QCheckBox ("View Running Settings?", parent);
	parent->connect (viewRunningSettings, &QCheckBox::stateChanged, [this]() {
		if (viewRunningSettings->isChecked ()) {
			// just changed to checked, so the settings should indicate the config settings still. 
			updateSettings ();
			updateWindowEnabledStatus ();
			updateDisplays ();
			currentlyUneditable = true;
		}
		else {
			updateWindowEnabledStatus ();
			updateDisplays ();
			currentlyUneditable = false;
		}
		});
	layout1->addWidget(controlAndorCameraCheck, 0);
	layout1->addWidget(viewRunningSettings, 0);

	QHBoxLayout* layout2 = new QHBoxLayout(this);
	layout2->setContentsMargins(0, 0, 0, 0);
	cameraModeCombo = new CQComboBox (parent);
	for (auto mode : AndorRunModes::allModes) {
		cameraModeCombo->addItem(qstr(AndorRunModes::toStr(mode)));
	}
	parent->connect (cameraModeCombo, qOverload<int> (&QComboBox::activated),
		[this, parent]() {
			if (!viewRunningSettings->isChecked ()) {
				updateCameraMode ();
				updateWindowEnabledStatus ();
			}
		});
	cameraModeCombo->setCurrentIndex(0);
	configSettings.andor.acquisitionMode = AndorRunModes::mode::Single;
	
	triggerCombo = new CQComboBox (parent);
	for (auto mode : AndorTriggerMode::allModes) {
		triggerCombo->addItem(qstr(AndorTriggerMode::toStr(mode)));
	}
	parent->connect (triggerCombo, qOverload<int> (&QComboBox::activated),
		[this, parent]() {
			if (!viewRunningSettings->isChecked ()) {
				updateTriggerMode ();
				updateWindowEnabledStatus ();
				auto& andorCore = parent->andorWin->getCamera();
				updateSettings();
				andorCore.setSettings(configSettings.andor);
				if (configSettings.andor.triggerMode == AndorTriggerMode::mode::Internal) {
					frameRateEdit->setEnabled(true);
					frameRateEdit->setStyleSheet("QLineEdit { background: rgb(255, 255, 255); }");
				}
				else {
					frameRateEdit->setEnabled(false);
					frameRateEdit->setStyleSheet("QLineEdit { background: rgb(204, 204, 204); }");
				}
				picSettingsObj.toggleExposureTimeEditGui(!(configSettings.andor.triggerMode == AndorTriggerMode::mode::ExternalExposure));
				try {
					andorCore.setCameraTriggerMode();
				}
				catch (ChimeraError& e) {
					parent->reportErr(e.qtrace());
				}
			}
		});
	triggerCombo->setCurrentIndex(0);
	configSettings.andor.triggerMode = AndorTriggerMode::mode::External;

	gainCombo = new CQComboBox(parent);
	for (auto mode : AndorGainMode::allModes) {
		gainCombo->addItem(qstr(AndorGainMode::toStr(mode)));
	}
	parent->connect(gainCombo, qOverload<int>(&QComboBox::activated),
		[this, parent]() {
			if (!viewRunningSettings->isChecked()) {
				updateGainMode();
				updateWindowEnabledStatus();
				auto& andorCore = parent->andorWin->getCamera();
				updateSettings();
				andorCore.setSettings(configSettings.andor);
				try {
					andorCore.setImageParametersToCamera();
					andorCore.setCameraGainMode();
				}
				catch (ChimeraError& e) {
					parent->reportErr(e.qtrace());
				}
				currentlyRunningSettings.frameRate = configSettings.andor.frameRate;
				updateMaxFrameRate(andorCore.getMaxFrameRate());
			}
		});
	gainCombo->setCurrentIndex(0);
	configSettings.andor.gainMode = AndorGainMode::mode::FastestFrameRate;

	//binningCombo = new CQComboBox(parent);
	//for (auto mode : AndorBinningMode::allModes) {
	//	binningCombo->addItem(qstr(AndorBinningMode::toStr(mode)));
	//}
	//binningCombo->setCurrentIndex(0);
	//parent->connect(binningCombo, qOverload<int>(&QComboBox::activated),
	//	[this, parent]() {
	//		if (!viewRunningSettings->isChecked()) {
	//			updateBinningMode();
	//			updateWindowEnabledStatus();
	//			auto& andorCore = parent->andorWin->getCamera();
	//			updateSettings();
	//			andorCore.setSettings(configSettings.andor);
	//			try {
	//				andorCore.setCameraBinningMode();
	//			}
	//			catch (ChimeraError& e) {
	//				parent->reportErr(e.qtrace());
	//			}
	//		}
	//	});
	//configSettings.andor.binningMode = AndorBinningMode::mode::oneByOne;

	layout2->addWidget(cameraModeCombo, 0);
	layout2->addWidget(triggerCombo, 0);
	layout2->addWidget(gainCombo, 0);

	QHBoxLayout* layout3 = new QHBoxLayout(this);
	layout3->setContentsMargins(0, 0, 0, 0);

	frameRateLabel = new QLabel("Frame Rate: ", parent);
	frameRateEdit = new CQLineEdit("1", parent);
	maxframeRateLabel = new QLabel("Max: -1", parent);
	connect(frameRateEdit, &CQLineEdit::returnPressed, [this, parent]() {
		auto& andorCore = parent->andorWin->getCamera();
		updateSettings();
		andorCore.setSettings(configSettings.andor);
		try {
			andorCore.setImageParametersToCamera();
			configSettings.andor.frameRate = andorCore.setFrameRate();
		}
		catch (ChimeraError& e) {
			parent->reportErr(e.qtrace());
		}
		currentlyRunningSettings.frameRate = configSettings.andor.frameRate;
		updateMaxFrameRate(andorCore.getMaxFrameRate());
		});

	layout3->addWidget(frameRateLabel, 0);
	layout3->addWidget(frameRateEdit, 0);
	layout3->addWidget(maxframeRateLabel, 0);

	//QHBoxLayout* layout3 = new QHBoxLayout(this);
	//layout3->setContentsMargins(0, 0, 0, 0);
	//frameTransferModeCombo = new CQComboBox(parent);
	//frameTransferModeCombo->setToolTip("Frame Transfer Mode OFF:\n"
	//	"Slower than when on. Cleans between images. Mechanical shutter may not be necessary.\n"
	//	"Frame Transfer Mode ON:\n"
	//	"Faster than when off. Does not clean between images, giving better background.\n"
	//	"Mechanical Shutter probably necessary unless imaging continuously.\n"
	//	"See iXonEM + hardware guide pg 42.\n");
	//frameTransferModeCombo->addItems({ "FTM: OFF!", "FTM: ON!" });
	//frameTransferModeCombo->setCurrentIndex(0);
	//configSettings.andor.frameTransferMode = 0;

	//verticalShiftSpeedCombo = new CQComboBox (parent);
	//for (auto speed : vertSpeeds){ 
	//	verticalShiftSpeedCombo->addItem ("VS: " + qstr(speed));
	//}
	//verticalShiftSpeedCombo->setCurrentIndex (1);
	//configSettings.andor.vertShiftSpeedSetting = 0;

	//horizontalShiftSpeedCombo = new CQComboBox (parent);
	//for (auto speed : horSpeeds){
	//	horizontalShiftSpeedCombo->addItem ("HS: " + qstr (speed));
	//}
	//horizontalShiftSpeedCombo->setCurrentIndex (0);
	//configSettings.andor.horShiftSpeedSetting = 0;

	//layout3->addWidget(frameTransferModeCombo, 0);
	//layout3->addWidget(verticalShiftSpeedCombo, 0);
	//layout3->addWidget(horizontalShiftSpeedCombo, 0);

	//QHBoxLayout* layout4 = new QHBoxLayout(this);
	//layout4->setContentsMargins(0, 0, 0, 0);
	//emGainBtn = new CQPushButton ("Set EM Gain (-1=OFF)", parent);
	//parent->connect (emGainBtn, &QPushButton::released, [parent]() {
	//		parent->andorWin->handleEmGainChange ();
	//	});
	//emGainEdit = new CQLineEdit ("-1", parent);
	//emGainEdit->setToolTip( "Set the state & gain of the EM gain of the camera. Enter a negative number to turn EM Gain"
	//					   " mode off. The program will immediately change the state of the camera after changing this"
	//					   " edit." );
	////
	//emGainDisplay = new QLabel ("OFF", parent);
	//// initialize settings.
	//configSettings.andor.emGainLevel = 0;
	//configSettings.andor.emGainModeIsOn = false;
	//layout4->addWidget(emGainBtn, 0);
	//layout4->addWidget(emGainEdit, 0);
	//layout4->addWidget(emGainDisplay, 0);

	QHBoxLayout* layout5 = new QHBoxLayout(this);
	layout5->setContentsMargins(0, 0, 0, 0);
	setTemperatureButton = new CQPushButton ("Set Camera Temperature (C)", parent);
	parent->connect (setTemperatureButton, &QPushButton::released, 
		[parent]() {
			parent->andorWin->passSetTemperaturePress ();
		});
	temperatureEdit = new CQLineEdit ("0", parent);
	temperatureDisplay = new QLabel ("", parent);
	//temperatureOffButton = new CQPushButton("OFF", parent);
	layout5->addWidget(setTemperatureButton, 0);
	layout5->addWidget(temperatureEdit, 0);
	layout5->addWidget(temperatureDisplay, 0);
	//layout5->addWidget(temperatureOffButton, 0);
	
	temperatureMsg = new QLabel("Temperature control is disabled", parent);


	picSettingsObj.initialize( parent );
	imageDimensionsObj.initialize(parent);

	QGridLayout* layout6 = new QGridLayout(this);
	//// Accumulation Time
	//accumulationCycleTimeLabel = new QLabel ("Accumulation Cycle Time", parent);
	//accumulationCycleTimeEdit = new CQLineEdit ("0.1", parent);
	//// Accumulation Number
	//accumulationNumberLabel = new QLabel ("Accumulation #", parent);
	//accumulationNumberEdit = new CQLineEdit ("1", parent);
	//// minimum kinetic cycle time (determined by camera)
	//minKineticCycleTimeLabel = new QLabel ("Minimum Kinetic Cycle Time (s)", parent);
	//minKineticCycleTimeDisp = new QLabel("---", parent);
	///// Kinetic Cycle Time
	//kineticCycleTimeLabel = new QLabel ("Kinetic Cycle Time (s)", parent);
	//kineticCycleTimeEdit = new CQLineEdit("0.1", parent);	
	//layout6->addWidget(accumulationCycleTimeLabel, 0, 0);
	//layout6->addWidget(accumulationCycleTimeEdit, 0, 1);
	//layout6->addWidget(accumulationNumberLabel, 0, 2);
	//layout6->addWidget(accumulationNumberEdit, 0, 3);
	//layout6->addWidget(minKineticCycleTimeLabel, 1, 0);
	//layout6->addWidget(minKineticCycleTimeDisp, 1, 1);
	//layout6->addWidget(kineticCycleTimeLabel, 1, 2);
	//layout6->addWidget(kineticCycleTimeEdit, 1, 3);

	calControl.initialize( parent );

	layout->addLayout(layout1);
	layout->addLayout(layout2);
	layout->addLayout(layout3);
	//layout->addLayout(layout4);
	layout->addLayout(layout5);
	layout->addWidget(temperatureMsg);
	layout->addWidget(&imageDimensionsObj);
	layout->addWidget(&picSettingsObj);
	layout->addLayout(layout6);
	layout->addWidget(&calControl);
	updateWindowEnabledStatus ();

	emit cameraModeCombo->activated(0);
	emit triggerCombo->activated(0);
	emit gainCombo->activated(0);
}


// note that this object doesn't actually store the camera state, it just uses it in passing to figure out whether 
// buttons should be on or off.
void AndorCameraSettingsControl::cameraIsOn(bool state){
	// Can't change em gain mode or camera settings once started.
	//emGainEdit->setEnabled( !state );
	setTemperatureButton->setEnabled ( !state );
	//temperatureOffButton->setEnabled ( !state );
}

void AndorCameraSettingsControl::setConfigSettings (AndorRunSettings inputSettings) {
	configSettings.andor = inputSettings;
	updateDisplays ();
}

void AndorCameraSettingsControl::updateDisplays () {
	auto optionsIn = viewRunningSettings->isChecked () ? currentlyRunningSettings : configSettings.andor;
	controlAndorCameraCheck->setChecked (optionsIn.controlCamera);
	frameRateEdit->setText(qstr(optionsIn.frameRate));
	//kineticCycleTimeEdit->setText (qstr (optionsIn.kineticCycleTime));
	//accumulationCycleTimeEdit->setText (qstr (optionsIn.accumulationTime));
	int ind = cameraModeCombo->findText (AndorRunModes::toStr (optionsIn.acquisitionMode).c_str ());
	if (ind != -1) {
		cameraModeCombo->setCurrentIndex (ind);
	}
	ind = triggerCombo->findText (AndorTriggerMode::toStr (optionsIn.triggerMode).c_str ());
	if (ind != -1) {
		triggerCombo->setCurrentIndex (ind);
	}
	ind = gainCombo->findText(AndorGainMode::toStr(optionsIn.gainMode).c_str());
	if (ind != -1) {
		gainCombo->setCurrentIndex(ind);
	}
	//ind = binningCombo->findText(AndorBinningMode::toStr(optionsIn.binningMode).c_str());
	//if (ind != -1) {
	//	binningCombo->setCurrentIndex(ind);
	//}
	//kineticCycleTimeEdit->setText (qstr (optionsIn.kineticCycleTime));
	//accumulationCycleTimeEdit->setText (qstr (optionsIn.accumulationTime * 1000.0));
	//accumulationNumberEdit->setText (qstr (optionsIn.accumulationNumber));
	temperatureEdit->setText (qstr (optionsIn.temperatureSetting));
	imageDimensionsObj.setImageParametersFromInput (optionsIn.imageSettings);

	//verticalShiftSpeedCombo->setCurrentIndex(optionsIn.vertShiftSpeedSetting);
	//horizontalShiftSpeedCombo->setCurrentIndex(optionsIn.horShiftSpeedSetting);
	//frameTransferModeCombo->setCurrentIndex(optionsIn.frameTransferMode);

	picSettingsObj.setContinuousMode(optionsIn.continuousMode, optionsIn.picsPerRepetition);
	picSettingsObj.setUnofficialExposures (optionsIn.exposureTimes);
	picSettingsObj.setUnofficialPicsPerRep (optionsIn.picsPerRepetition);

}


void AndorCameraSettingsControl::setRunSettings(AndorRunSettings inputSettings){
	currentlyRunningSettings = inputSettings;
	picSettingsObj.setUnofficialExposures ( inputSettings.exposureTimes );
	picSettingsObj.setUnofficialPicsPerRep ( inputSettings.picsPerRepetition );
	///
	updateDisplays ();
}


void AndorCameraSettingsControl::handleSetTemperaturePress(){
	try{
		configSettings.andor.temperatureSetting = boost::lexical_cast<int>(str(temperatureEdit->text ()));
	}
	catch ( boost::bad_lexical_cast&){
		throwNested("Error: Couldn't convert temperature input to a double! Check for unusual characters.");
	}
}

void AndorCameraSettingsControl::updateTriggerMode( ){
	if (currentlyUneditable) {
		return;
	}
	int itemIndex = triggerCombo->currentIndex( );
	if ( itemIndex == -1 ){
		return;
	}
	configSettings.andor.triggerMode = AndorTriggerMode::fromStr(str(triggerCombo->currentText ()));
}

unsigned AndorCameraSettingsControl::getHsSpeed () {
	//return horizontalShiftSpeedCombo->currentIndex ();
	return -1;
}

unsigned AndorCameraSettingsControl::getVsSpeed () {
	//return verticalShiftSpeedCombo->currentIndex ();
	return -1;
}

unsigned AndorCameraSettingsControl::getFrameTransferMode() {
	//return frameTransferModeCombo->currentIndex();
	return -1;
}

void AndorCameraSettingsControl::updateSettings(){
	if (currentlyUneditable) {
		return;
	}
	// update all settings with current values from controls
	configSettings.andor.controlCamera =		controlAndorCameraCheck->isChecked ();
	configSettings.andor.exposureTimes =		picSettingsObj.getUsedExposureTimes( );
	configSettings.thresholds =				picSettingsObj.getThresholds( );
	configSettings.palleteNumbers =			picSettingsObj.getPictureColors( );
	configSettings.andor.continuousMode = picSettingsObj.getContinuousMode();
	configSettings.andor.picsPerRepetition =	picSettingsObj.getPicsPerRepetition( );
	
	configSettings.andor.imageSettings = readImageParameters( );
	configSettings.andor.frameRate = getFrameRate();
	//configSettings.andor.kineticCycleTime = getKineticCycleTime( );
	//configSettings.andor.accumulationTime = getAccumulationCycleTime( );
	//configSettings.andor.accumulationNumber = getAccumulationNumber( );

	updateCameraMode( );
	updateTriggerMode( );
	updateGainMode();
	//updateBinningMode();

	//configSettings.andor.horShiftSpeedSetting = getHsSpeed ();
	//configSettings.andor.vertShiftSpeedSetting = getVsSpeed ();
	//configSettings.andor.frameTransferMode = getFrameTransferMode();
}

std::array<softwareAccumulationOption, 4> AndorCameraSettingsControl::getSoftwareAccumulationOptions ( ){
	return picSettingsObj.getSoftwareAccumulationOptions();
}

AndorCameraSettings AndorCameraSettingsControl::getConfigSettings(){
	updateSettings ();
	return configSettings;
}

AndorRunSettings AndorCameraSettingsControl::getRunningSettings () {
	return currentlyRunningSettings;
}

AndorCameraSettings AndorCameraSettingsControl::getCalibrationSettings( ){
	AndorCameraSettings callOptions;
	callOptions.andor.acquisitionMode = AndorRunModes::mode::Kinetic;
	//callOptions.andor.emGainLevel = 0;
	//callOptions.andor.emGainModeIsOn = false;
	callOptions.andor.exposureTimes = { float(10e-3) };
	// want to calibrate the image area to be used in the experiment, so...
	callOptions.andor.imageSettings = imageDimensionsObj.readImageParameters( );
	callOptions.andor.frameRate = 10.0;
	//callOptions.andor.kineticCycleTime = 10e-3f;
	callOptions.andor.picsPerRepetition = 1;
	//callOptions.andor.readMode = 4;
	callOptions.andor.repetitionsPerVariation = 100;
	callOptions.andor.temperatureSetting = -60;
	callOptions.andor.totalVariations = 1;
	callOptions.andor.triggerMode = AndorTriggerMode::mode::External;
	return callOptions;
}

bool AndorCameraSettingsControl::getAutoCal( ){
	return calControl.autoCal( );
}

bool AndorCameraSettingsControl::getUseCal( ){
	return calControl.use( );
}

//void AndorCameraSettingsControl::setEmGain( bool emGainCurrentlyOn, int currentEmGainLevel ){
//	auto emGainText = emGainEdit->text();
//	if ( emGainText == "" ){
//		// set to off.
//		emGainText = "-1";
//	}
//	int emGain;
//	try{
//		emGain = boost::lexical_cast<int>(str(emGainText));
//	}
//	catch ( boost::bad_lexical_cast&){
//		throwNested("ERROR: Couldn't convert EM Gain text to integer! Aborting!");
//	}
//	// < 0 corresponds to NOT USING EM GAIN (using conventional gain).
//	if (emGain < 0){
//		configSettings.andor.emGainModeIsOn = false;
//		configSettings.andor.emGainLevel = 0;
//		emGainDisplay->setText("OFF");
//	}
//	else{
//		configSettings.andor.emGainModeIsOn = true;
//		configSettings.andor.emGainLevel = emGain;
//		emGainDisplay->setText(cstr("Gain: X" + str(configSettings.andor.emGainLevel)));
//	}
//	// Change the andor settings.
//	std::string promptMsg = "";
//	if ( emGainCurrentlyOn != configSettings.andor.emGainModeIsOn ){
//		promptMsg += "Set Andor EM Gain State to " + str(configSettings.andor.emGainModeIsOn ? "ON" : "OFF");
//	}
//	if ( currentEmGainLevel != configSettings.andor.emGainLevel ){
//		if ( promptMsg != "" ){
//			promptMsg += ", ";
//		}
//		promptMsg += "Set Andor EM Gain Level to " + str(configSettings.andor.emGainLevel);
//	}
//	if ( promptMsg != "" ){
//		promptMsg += "?";
//		auto result = QMessageBox::question (nullptr, "Andor Settings", qstr(promptMsg));
//		if ( result == QMessageBox::No ){
//			thrower ( "Aborting camera settings update at EM Gain update!" );
//		}
//	}
//}

void AndorCameraSettingsControl::setVariationNumber(unsigned varNumber){
	AndorRunSettings& andorSettings = configSettings.andor;
	andorSettings.totalVariations = varNumber;
	if ( andorSettings.totalPicsInExperiment() > INT_MAX){
		thrower ( "ERROR: Trying to take too many pictures! Maximum picture number is " + str( INT_MAX ) );
	}
}

void AndorCameraSettingsControl::setRepsPerVariation(unsigned repsPerVar){
	AndorRunSettings& andorSettings = configSettings.andor;
	andorSettings.repetitionsPerVariation = repsPerVar;
	if ( andorSettings.totalPicsInExperiment() > INT_MAX){
		thrower ( "ERROR: Trying to take too many pictures! Maximum picture number is " + str( INT_MAX ) );
	}
}

void AndorCameraSettingsControl::changeTemperatureDisplay( AndorTemperatureStatus stat ){
	temperatureDisplay->setText ( qstr( stat.temperatureSetting ) );
	temperatureMsg->setText (  qstr( stat.msg ) );
	QString colorcode = QVariant(stat.colorCode).toString();
	for (auto l : { temperatureDisplay ,temperatureMsg }) {
		l->setStyleSheet("QLabel { background-color :" + colorcode + " ; }");
	}
}

void AndorCameraSettingsControl::updateRunSettingsFromPicSettings( ){
	configSettings.andor.exposureTimes = picSettingsObj.getUsedExposureTimes( );
	configSettings.andor.picsPerRepetition = picSettingsObj.getPicsPerRepetition( );
	if ( configSettings.andor.totalPicsInExperiment ( ) > INT_MAX ){
		thrower ( "ERROR: Trying to take too many pictures! Maximum picture number is " + str( INT_MAX ) );
	}
}

void AndorCameraSettingsControl::handlePictureSettings(){
	picSettingsObj.handleOptionChange();
	updateRunSettingsFromPicSettings( );
}

double AndorCameraSettingsControl::getFrameRate()
{
	if (!frameRateEdit) {
		return 0;
	}
	try{
		configSettings.andor.frameRate = boost::lexical_cast<float>( str(frameRateEdit->text ()) );
		frameRateEdit->setText( qstr( configSettings.andor.frameRate) );
	}
	catch ( boost::bad_lexical_cast& ){
		configSettings.andor.frameRate = 10.0;
		frameRateEdit->setText ( qstr( configSettings.andor.frameRate) );
		throwNested( "Please enter a valid double for the frame rate." );
	}
	return configSettings.andor.frameRate;
}

//double AndorCameraSettingsControl::getKineticCycleTime( ){
//	if (!kineticCycleTimeEdit) {
//		return 0;
//	}
//	try{
//		configSettings.andor.kineticCycleTime = boost::lexical_cast<float>( str(kineticCycleTimeEdit->text ()) );
//		kineticCycleTimeEdit->setText( cstr( configSettings.andor.kineticCycleTime ) );
//	}
//	catch ( boost::bad_lexical_cast& ){
//		configSettings.andor.kineticCycleTime = 0.1f;
//		kineticCycleTimeEdit->setText ( cstr( configSettings.andor.kineticCycleTime ) );
//		throwNested( "Please enter a valid float for the kinetic cycle time." );
//	}
//	return configSettings.andor.kineticCycleTime;
//}
//
//double AndorCameraSettingsControl::getAccumulationCycleTime( ){
//	if (!accumulationCycleTimeEdit){
//		return 0;
//	}
//	try	{
//		configSettings.andor.accumulationTime = boost::lexical_cast<float>( str(accumulationCycleTimeEdit->text ()) );
//		accumulationCycleTimeEdit->setText( cstr( configSettings.andor.accumulationTime ) );
//	}
//	catch ( boost::bad_lexical_cast& ){
//		configSettings.andor.accumulationTime = 0.1f;
//		accumulationCycleTimeEdit->setText( cstr( configSettings.andor.accumulationTime ) );
//		throwNested( "Please enter a valid float for the accumulation cycle time." );
//	}
//	return configSettings.andor.accumulationTime;
//}
//
//unsigned AndorCameraSettingsControl::getAccumulationNumber( ){
//	if (!accumulationNumberEdit){
//		return 0;
//	}
//	try	{
//		configSettings.andor.accumulationNumber = boost::lexical_cast<long>( str(accumulationNumberEdit->text ()) );
//		accumulationNumberEdit->setText( cstr( configSettings.andor.accumulationNumber ) );
//	}
//	catch ( boost::bad_lexical_cast& ){
//		configSettings.andor.accumulationNumber = 1;
//		accumulationNumberEdit->setText( cstr( configSettings.andor.accumulationNumber ) );
//		throwNested( "Please enter a valid float for the Accumulation number." );
//	}
//	return configSettings.andor.accumulationNumber;
//}

void AndorCameraSettingsControl::updatePicSettings ( andorPicSettingsGroup settings ){
	picSettingsObj.updateAllSettings ( settings );
}

//void AndorCameraSettingsControl::updateImageDimSettings( imageParameters settings ){
//	imageDimensionsObj.setImageParametersFromInput ( settings );
//}

andorPicSettingsGroup AndorCameraSettingsControl::getPictureSettingsFromConfig (ConfigStream& configFile ){
	return PictureSettingsControl::getPictureSettingsFromConfig ( configFile );
}

void AndorCameraSettingsControl::handleSaveConfig(ConfigStream& saveFile){
	updateSettings ();
	saveFile << "CAMERA_SETTINGS\n";
	saveFile << "/*Control Andor:*/\t\t\t" << configSettings.andor.controlCamera << "\n";
	saveFile << "/*Trigger Mode:*/\t\t\t" << AndorTriggerMode::toStr(configSettings.andor.triggerMode) << "\n";
	//saveFile << "/*EM-Gain Is On:*/\t\t\t" << configSettings.andor.emGainModeIsOn << "\n";
	//saveFile << "/*EM-Gain Level:*/\t\t\t" << configSettings.andor.emGainLevel << "\n";
	saveFile << "/*Acquisition Mode:*/\t\t" << AndorRunModes::toStr (configSettings.andor.acquisitionMode) << "\n";
	saveFile << "/*Gain Mode:*/\t\t\t\t" << AndorGainMode::toStr(configSettings.andor.gainMode) << "\n";
	saveFile << "/*Frame Rate:*/\t\t\t\t" << configSettings.andor.frameRate << "\n";
	//saveFile << "/*Kinetic Cycle Time:*/\t\t" << configSettings.andor.kineticCycleTime << "\n";
	//saveFile << "/*Accumulation Time:*/\t\t" << configSettings.andor.accumulationTime << "\n";
	//saveFile << "/*Accumulation Number:*/\t" << configSettings.andor.accumulationNumber << "\n";
	saveFile << "/*Camera Temperature:*/\t\t" << configSettings.andor.temperatureSetting << "\n";
	saveFile << "/*Number of Exposures:*/\t" << configSettings.andor.exposureTimes.size ( ) 
			 << "\n/*Exposure Times:*/\t\t\t";
	for ( auto exposure : configSettings.andor.exposureTimes ){
		saveFile << exposure << " ";
	}
	saveFile << "\n/*Andor Continuous Mode:*/\t" << configSettings.andor.continuousMode;
	saveFile << "\n/*Andor Pics Per Rep:*/\t\t" << configSettings.andor.picsPerRepetition;
	//saveFile << "\n/*Horizontal Shift Speed*/\t" << configSettings.andor.horShiftSpeedSetting;
	//saveFile << "\n/*Vertical Shift Speed*/\t" << configSettings.andor.vertShiftSpeedSetting;
	saveFile << "\nEND_CAMERA_SETTINGS\n";
	picSettingsObj.handleSaveConfig(saveFile);
	imageDimensionsObj.handleSave (saveFile);
}


void AndorCameraSettingsControl::updateCameraMode( ){
	/* updates settings.andor.cameraMode based on combo selection, then updates 
		settings.andor.acquisitionMode and other settings depending on the mode.
	*/
	if (currentlyUneditable) {
		return;
	}
	int sel = cameraModeCombo->currentIndex( );
	if ( sel == -1 ){
		return;
	}
	//std::string txt (str(cameraModeCombo->currentText()));
	configSettings.andor.acquisitionMode = AndorRunModes::fromStr(str(cameraModeCombo->currentText()));
	//if ( txt == AndorRunModes::toStr (AndorRunModes::mode::Single)){
	//	configSettings.andor.acquisitionMode = AndorRunModes::mode::Single;
	//	//configSettings.andor.repetitionsPerVariation = INT_MAX;
	//}
	//else if ( txt == AndorRunModes::toStr ( AndorRunModes::mode::Kinetic )){
	//	configSettings.andor.acquisitionMode = AndorRunModes::mode::Kinetic;
	//}
	//else if ( txt == AndorRunModes::toStr ( AndorRunModes::mode::Accumulate )){
	//	configSettings.andor.acquisitionMode = AndorRunModes::mode::Accumulate;
	//}
	//else{
	//	thrower  ( "ERROR: unrecognized combo for andor run mode text???" );
	//}
}

void AndorCameraSettingsControl::updateGainMode()
{
	if (currentlyUneditable) {
		return;
	}
	int sel = gainCombo->currentIndex();
	if (sel == -1) {
		return;
	}
	configSettings.andor.gainMode = AndorGainMode::fromStr(str(gainCombo->currentText()));
}

//void AndorCameraSettingsControl::updateBinningMode()
//{
//	if (currentlyUneditable) {
//		return;
//	}
//	int sel = binningCombo->currentIndex();
//	if (sel == -1) {
//		return;
//	}
//	configSettings.andor.binningMode = AndorBinningMode::fromStr(str(binningCombo->currentText()));
//	//imageDimensionsObj.setBinningMode(configSettings.andor.binningMode);
//}

void AndorCameraSettingsControl::updateWindowEnabledStatus (){
	controlAndorCameraCheck->setEnabled (!viewRunningSettings->isChecked ());
	cameraModeCombo->setEnabled (!viewRunningSettings->isChecked ());
	//emGainEdit->setEnabled (!viewRunningSettings->isChecked ());
	//emGainBtn->setEnabled (!viewRunningSettings->isChecked ());
	triggerCombo->setEnabled (!viewRunningSettings->isChecked ());
	gainCombo->setEnabled(!viewRunningSettings->isChecked());
	//binningCombo->setEnabled(!viewRunningSettings->isChecked());

	auto settings = getConfigSettings ();
	frameRateEdit->setEnabled(settings.andor.triggerMode == AndorTriggerMode::mode::Internal
		&& !viewRunningSettings->isChecked());
	//accumulationCycleTimeEdit->setEnabled(settings.andor.acquisitionMode == AndorRunModes::mode::Accumulate 
	//	&& !viewRunningSettings->isChecked ());
	//accumulationNumberEdit->setEnabled (settings.andor.acquisitionMode == AndorRunModes::mode::Accumulate 
	//	&& !viewRunningSettings->isChecked ());
	//kineticCycleTimeEdit->setEnabled (settings.andor.acquisitionMode == AndorRunModes::mode::Kinetic 
	//	&& !viewRunningSettings->isChecked ());
	
	imageDimensionsObj.updateEnabledStatus (viewRunningSettings->isChecked ());
	picSettingsObj.setEnabledStatus (viewRunningSettings->isChecked ());

}

//void AndorCameraSettingsControl::updateMinKineticCycleTime( double time ){
//	minKineticCycleTimeDisp->setText( cstr( time ) );
//}

void AndorCameraSettingsControl::updateMaxFrameRate(double framerate)
{
	maxframeRateLabel->setText("Max: " + qstr(framerate, 3));
}

imageParameters AndorCameraSettingsControl::readImageParameters(){
	return imageDimensionsObj.readImageParameters( );
}

void AndorCameraSettingsControl::setImageParameters(imageParameters newSettings){
	imageDimensionsObj.setImageParametersFromInput(newSettings);
}

void AndorCameraSettingsControl::checkIfReady(){
	if ( picSettingsObj.getUsedExposureTimes().size() == 0 ){
		thrower ("Please Set at least one exposure time.");
	}
	if ( !imageDimensionsObj.checkReady() ){
		thrower ("Please set the image parameters.");
	}
	if ( configSettings.andor.picsPerRepetition <= 0 ){
		thrower ("ERROR: Please set the number of pictures per repetition to a positive non-zero value.");
	}
	if ( configSettings.andor.acquisitionMode == AndorRunModes::mode::Kinetic ){
		if ( configSettings.andor.frameRate == 0 && configSettings.andor.triggerMode == AndorTriggerMode::mode::Internal ){
			thrower ("ERROR: Since you are running in internal trigger mode, please Set a frame rate.");
		}
		if ( configSettings.andor.repetitionsPerVariation <= 0 ){
			thrower ("ERROR: Please set the \"Repetitions Per Variation\" variable to a positive non-zero value.");
		}
		if ( configSettings.andor.totalVariations <= 0 ){
			thrower ("ERROR: Please set the number of variations to a positive non-zero value.");
		}
	}
	//if ( configSettings.andor.acquisitionMode == AndorRunModes::mode::Accumulate ){
	//	if ( configSettings.andor.accumulationNumber <= 0 ){
	//		thrower ("ERROR: Please set the current Accumulation Number to a positive non-zero value.");
	//	}
	//	if ( configSettings.andor.accumulationTime <= 0 ){
	//		thrower ("ERROR: Please set the current Accumulation Time to a positive non-zero value.");
	//	}
	//}
}

void AndorCameraSettingsControl::handelSaveMasterConfig ( std::stringstream& configFile ){
	imageParameters settings = getConfigSettings ( ).andor.imageSettings;
	configFile << settings.left << " " << settings.right << " " << settings.horizontalBinning << " ";
	configFile << settings.bottom << " " << settings.top << " " << settings.verticalBinning << "\n";
	// introduced in version 2.2
	configFile << getAutoCal ( ) << " " << getUseCal ( ) << "\n";
}

void AndorCameraSettingsControl::handleOpenMasterConfig ( ConfigStream& configStream, QtAndorWindow* camWin ){
	imageParameters settings = getConfigSettings ( ).andor.imageSettings;
	std::string tempStr;
	try	{
		configStream >> tempStr;
		settings.left = boost::lexical_cast<long> ( tempStr );
		configStream >> tempStr;
		settings.right = boost::lexical_cast<long> ( tempStr );
		configStream >> tempStr;
		settings.horizontalBinning = boost::lexical_cast<long> ( tempStr );
		configStream >> tempStr;
		settings.bottom = boost::lexical_cast<long> ( tempStr );
		configStream >> tempStr;
		settings.top = boost::lexical_cast<long> ( tempStr );
		configStream >> tempStr;
		settings.verticalBinning = boost::lexical_cast<long> ( tempStr );
		setImageParameters ( settings );
	}
	catch ( boost::bad_lexical_cast& ){
		throwNested ( "ERROR: Bad value (i.e. failed to convert to long) seen in master configueration file while attempting "
				  "to load camera dimensions!" );
	}
	bool autoCal, useCal;
	configStream >> autoCal >> useCal;
	calControl.setAutoCal ( autoCal );
	calControl.setUse ( useCal );
}


std::vector<Matrix<long>> AndorCameraSettingsControl::getImagesToDraw ( const std::vector<Matrix<long>>& rawData ){
	std::vector<Matrix<long>> imagesToDraw ( rawData.size ( ) );
	auto options = picSettingsObj.getDisplayTypeOptions ( );
	for ( auto picNum : range ( rawData.size ( ) ) ){
		if ( !options[ picNum ].isDiff ){
			imagesToDraw[ picNum ] = rawData[ picNum ];
		}
		else{
			// the whichPic variable is 1-indexed.
			if ( options[ picNum ].whichPicForDif >= rawData.size ( ) ){
				imagesToDraw[ picNum ] = rawData[ picNum ];
			}
			else{
				imagesToDraw[ picNum ] = Matrix<long>(rawData[picNum].getRows(), rawData[picNum].getCols(), 0);
				for ( auto i : range ( rawData[ picNum ].size ( ) ) ){
					imagesToDraw[ picNum ].data[ i ] = rawData[ picNum ].data[ i ] - rawData[ options[ picNum ].whichPicForDif - 1 ].data[ i ];
				}
			}
		}
	}
	return imagesToDraw;
}

