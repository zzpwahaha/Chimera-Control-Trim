#include "stdafx.h"
#include "QtAndorWindow.h"
#include <qdesktopwidget.h>
#include <PrimaryWindows/QtScriptWindow.h>
#include <PrimaryWindows/QtAndorWindow.h>
#include <PrimaryWindows/QtAuxiliaryWindow.h>
#include <PrimaryWindows/QtMakoWindow.h>
#include <PrimaryWindows/QtMainWindow.h>
#include <RealTimeDataAnalysis/AnalysisThreadWorker.h>
#include <Rearrangement/AtomCruncherWorker.h>
#include <ExperimentThread/ExpThreadWorker.h>
#include <QThread.h>
#include <qelapsedtimer.h>
#include <qdebug.h>


QtAndorWindow::QtAndorWindow (QWidget* parent) : IChimeraQtWindow (parent),
	andorSettingsCtrl (),
	dataHandler (DATA_SAVE_LOCATION, this),
	andor (ANDOR_SAFEMODE),
	pics (false, "ANDOR_PICTURE_MANAGER", false, Qt::SmoothTransformation),
	analysisHandler (this)
{
	
	setWindowTitle ("Andor Window");
}

QtAndorWindow::~QtAndorWindow (){}

int QtAndorWindow::getDataCalNum () {
	return dataHandler.getCalibrationFileIndex ();
}

void QtAndorWindow::initializeWidgets (){
	statBox = new ColorBox(this, mainWin->getDevices());
	QWidget* centralWidget = new QWidget();
	setCentralWidget(centralWidget);
	QHBoxLayout* layout = new QHBoxLayout(centralWidget);

	QVBoxLayout* layout1 = new QVBoxLayout(this);
	layout1->setContentsMargins(0, 0, 0, 0);
	andor.initializeClass(this, &atomCrunchThreadActive, &imageTimes);
	alerts.alertMainThread (0);
	alerts.initialize (this);
	analysisHandler.initialize (this);
	andorSettingsCtrl.initialize ( this, std::vector<std::string>()/*andor.getVertShiftSpeeds()*/, std::vector<std::string>()/*andor.getHorShiftSpeeds()*/);
	alerts.setMaximumWidth(450);
	analysisHandler.setMaximumSize(450, 300);
	andorSettingsCtrl.setMaximumWidth(450);
	layout1->addWidget(&alerts);
	layout1->addWidget(&analysisHandler);
	layout1->addWidget(&andorSettingsCtrl);
	layout1->addStretch(0);

	QVBoxLayout* layout2 = new QVBoxLayout(this);
	layout2->setContentsMargins(0, 0, 0, 0);
	stats.initialize (this);
	layout2->addWidget(&stats);
	for (auto pltInc : range (6)){
		mainAnalysisPlots.push_back (new QCustomPlotCtrl(1, plotStyle::BinomialDataPlot, { 0,0,0,0 }, false, false));
		mainAnalysisPlots.back()->init(this, "INACTIVE");
		mainAnalysisPlots.back()->plot->setMinimumSize(350, 130);
		mainAnalysisPlots.back()->plot->setMaximumSize(450, 140);
		layout2->addWidget(mainAnalysisPlots.back()->plot);
	}
	layout2->addStretch(1);

	QVBoxLayout* layout3 = new QVBoxLayout(this);
	layout3->setContentsMargins(0, 0, 0, 0);
	timer.initialize (this);
	timer.setMinimumWidth(750);
	pics.initialize (this);
	// end of literal initialization calls
	//pics.setSinglePicture (andorSettingsCtrl.getConfigSettings ().andor.imageSettings);
	timer.setMaximumHeight(45);
	layout3->addWidget(&timer);
	layout3->addWidget(&pics, 1);
	layout3->addStretch();



	andor.setSettings (andorSettingsCtrl.getConfigSettings ().andor);
	layout->addLayout(layout1, 0);
	layout->addLayout(layout2, 0);
	layout->addLayout(layout3, 0);
	
	//pics.setMultiplePictures(imageParameters(), 4);
	//pics.setSinglePicture(imageParameters());


	QTimer* timer = new QTimer (this);
	connect (timer, &QTimer::timeout, [this]() {
		auto temp = andor.getTemperature ();
		andorSettingsCtrl.changeTemperatureDisplay (temp); 
		});
	
	timer->start (2000);
}

void QtAndorWindow::manualArmCamera () {
	try {
		double time;
		andor.armCamera (time);
	}
	catch (ChimeraError & err) {
		reportErr (qstr (err.trace ()));
	}
}

void QtAndorWindow::handlePrepareForAcq (AndorRunSettings* lparam, analysisSettings aSettings){
	try {
		reportStatus ("Preparing Andor Window for Acquisition...\n");
		currentPictureNum = 0;
		currentRawPictures.clear();
		AndorRunSettings* settings = (AndorRunSettings*)lparam;
		analysisHandler.setRunningSettings (aSettings);
		armCameraWindow (settings);
		completeCruncherStart ();
		completePlotterStart ();
	}
	catch (ChimeraError & err) {
		reportErr (qstr (err.trace ()));
	}
}

void QtAndorWindow::handlePlotPop (unsigned id){
	for (auto& plt : mainAnalysisPlots)	{
	}
}

void QtAndorWindow::refreshPics()
{
	pics.setMultiplePictures(andorSettingsCtrl.getConfigSettings().andor.imageSettings, 4);
	pics.setSinglePicture(andorSettingsCtrl.getConfigSettings().andor.imageSettings);
}

void QtAndorWindow::displayAnalysisGrid(atomGrid grids)
{
	try {
		for (auto& pic : pics.pictures) {
			pic.drawAnalysisMarkers(grids);
		}
	}
	catch (ChimeraError& err) {
		reportErr(qstr(err.trace()));
	}

}

void QtAndorWindow::removeAnalysisGrid()
{
	for (auto& pic : pics.pictures) {
		pic.removeAnalysisMarkers();
	}
}

bool QtAndorWindow::wasJustCalibrated (){
	return justCalibrated;
}

bool QtAndorWindow::wantsAutoCal (){
	return andorSettingsCtrl.getAutoCal ();
}

void QtAndorWindow::writeVolts (unsigned currentVoltNumber, std::vector<float64> data){
	try	{
		dataHandler.writeVolts (currentVoltNumber, data);
	}
	catch (ChimeraError& err){
		reportErr (qstr (err.trace ()));
	}
}

void QtAndorWindow::handleImageDimsEdit (){
	try {
		pics.setParameters (andorSettingsCtrl.getConfigSettings ().andor.imageSettings);
		QPainter painter (this);
		pics.redrawPictures (selectedPixel, analysisHandler.getRunningSettings().grids, true, mostRecentPicNum, painter); 
	}
	catch (ChimeraError& err){
		reportErr (qstr (err.trace ()));
	}
}

//void QtAndorWindow::handleEmGainChange (){
//	try {
//		auto runSettings = andor.getAndorRunSettings ();
//		andorSettingsCtrl.setEmGain (runSettings.emGainModeIsOn, runSettings.emGainLevel);
//		auto settings = andorSettingsCtrl.getConfigSettings ();
//		runSettings.emGainModeIsOn = settings.andor.emGainModeIsOn;
//		runSettings.emGainLevel = settings.andor.emGainLevel;
//		andor.setSettings (runSettings);
//		// and immediately change the EM gain mode.
//		try	{
//			andor.setGainMode ();
//		}
//		catch (ChimeraError& err){
//			// this can happen e.g. if the camera is aquiring.
//			reportErr (qstr (err.trace ()));
//		}
//	}
//	catch (ChimeraError err){
//		reportErr (qstr (err.trace ()));
//	}
//}


std::string QtAndorWindow::getSystemStatusString (){
	std::string statusStr;
	statusStr = "\nAndor Camera:\n";
	if (!ANDOR_SAFEMODE){
		statusStr += "\tCode System is Active!\n";
		statusStr += "\t" + andor.getSystemInfo ();
		statusStr += "\t";
		statusStr += "Attached trigger line is \n\t\t";
		{
			statusStr += "(" + str(ANDOR_TRIGGER_LINE.first) + "," + str(ANDOR_TRIGGER_LINE.second) + ") ";
		}
		statusStr += "\n";
	}
	else{
		statusStr += "\tCode System is disabled! Enable in \"constants.h\"\n";
	}
	return statusStr;
}

void QtAndorWindow::windowSaveConfig (ConfigStream& saveFile){
	andorSettingsCtrl.handleSaveConfig (saveFile);
	pics.handleSaveConfig (saveFile);
	analysisHandler.handleSaveConfig (saveFile);
}

void QtAndorWindow::windowOpenConfig (ConfigStream& configFile){
	AndorRunSettings camSettings;
	try	{
		ConfigSystem::stdGetFromConfig (configFile, andor, camSettings);
		andorSettingsCtrl.setConfigSettings (camSettings);
		andorSettingsCtrl.setImageParameters (camSettings.imageSettings);
		andorSettingsCtrl.updateRunSettingsFromPicSettings ();
	}
	catch (ChimeraError& err){
		reportErr (qstr("Failed to get Andor Camera Run settings from file! " + err.trace ()));
	}
	try	{
		auto picSettings = ConfigSystem::stdConfigGetter (configFile, "PICTURE_SETTINGS",
			AndorCameraSettingsControl::getPictureSettingsFromConfig);
		andorSettingsCtrl.updatePicSettings (picSettings);
	}
	catch (ChimeraError& err)	{
		reportErr (qstr ("Failed to get Andor Camera Picture settings from file! " + err.trace ()));
	}
	try	{
		ConfigSystem::standardOpenConfig (configFile, pics.configDelim, &pics);
	}
	catch (ChimeraError&)	{
		reportErr ("Failed to load picture settings from config!");
	}
	try	{
		ConfigSystem::standardOpenConfig (configFile, "DATA_ANALYSIS", &analysisHandler);
	}
	catch (ChimeraError&){
		reportErr ("Failed to load Data Analysis settings from config!");
	}
	try	{
		pics.resetPictureStorage ();
		std::array<int, 4> nums = andorSettingsCtrl.getConfigSettings ().palleteNumbers;
		pics.setPalletes (nums);
	}
	catch (ChimeraError& e){
		reportErr (qstr ("Andor Camera Window failed to read parameters from the configuration file.\n\n" + e.trace ()));
	}
	analysisHandler.updateUnofficialPicsPerRep(andorSettingsCtrl.getConfigSettings().andor.picsPerRepetition,
		andorSettingsCtrl.getConfigSettings().andor.continuousMode);
}

void QtAndorWindow::abortCameraRun (bool askDelete){
	int status = andor.queryStatus ();
	if (ANDOR_SAFEMODE)	{
		// simulate as if you needed to abort.
		status = DRV_ACQUIRING;
	}
	if (true/*status == DRV_ACQUIRING*/){
		//andor.abortAcquisition ();
		// since the abortion can happen when the threadworker is waitForAcquisition, need to queue a buffer for it to get out of the wait
		// if has a trigger for andor, also need to attach a trigger for it
		//andor.updatePictureNumber(currentPictureNum + 1);
		//andor.setIsRunningState(false);
		//andor.queueBuffers();
		//Sleep(20);
		////auxWin->getTtlCore().FPGAForcePulse(auxWin->getTtlSystem().getCurrentStatus(), ANDOR_TRIGGER_LINE, 0.5);
		//Sleep(600); // just for  4fps during test, when run exp, probably not need this long
		andor.onFinish();
		qDebug() << "QtAndorWindow::abortCameraRun: Andor camera acquisition aborted, does WaitForAcquisition automatically release the hold? Tested and the answer is NO!";
		timer.setTimerDisplay ("Aborted");
		andor.setIsRunningState (false);
		// double set the cruncher thread flag just to be sure, this should be set in grabber already
		atomCrunchThreadActive = false;
		// camera is no longer running.
		try	{
			dataHandler.normalCloseFile ();
		}
		catch (ChimeraError& err)	{
			reportErr (qstr (err.trace ()));
		}

		if (askDelete/*andor.getAndorRunSettings ().acquisitionMode != AndorRunModes::mode::Video*/){
			auto answer = QMessageBox::question(this, qstr("Delete Data?"), qstr("Acquisition Aborted. Delete Data "
				"file (data_" + str (dataHandler.getDataFileNumber ()) + ".h5) for this run?"));
			if (answer == QMessageBox::Yes){
				try	{
					dataHandler.deleteFile ();
				}
				catch (ChimeraError& err) {
					reportErr (qstr (err.trace ()));
				}
			}
		}
	}
	else if (status == DRV_IDLE) {
		andor.setIsRunningState (false);
	}
}

bool QtAndorWindow::cameraIsRunning (){
	return andor.isRunning ();
}

void QtAndorWindow::onCameraProgress(NormalImage picGrabbed){
	auto timerE = QElapsedTimer();
	timerE.start();
	unsigned long long picNumReported = picGrabbed.picStat.picNum;
	unsigned picNum = currentPictureNum;
	currentPictureNum++;
	if (picNum % 2 == 1){
		mainThreadStartTimes.push_back (std::chrono::high_resolution_clock::now ());
	}
	AndorRunSettings curSettings = andor.getAndorRunSettings ();
	if (picNumReported != picNum){
		reportErr("WARNING: picture number reported by andor isn't matching the"
			"camera window record?!?!?!?!?");
	}
	if (picNum % curSettings.picsPerRepetition == 0) {
		currentRawPictures.clear();
		currentRawPictures.reserve(curSettings.picsPerRepetition);
	}
	if (curSettings.continuousMode) {
		currentRawPictures.clear(); // continuesMode only stores the latest image, otherwise need to adapt andorSettingsCtrl.getImagesToDraw
	}
	currentRawPictures.push_back(picGrabbed.image);
	auto& rawPicData = currentRawPictures;

	std::vector<Matrix<long>> calPicData (rawPicData.size ());
	if (andorSettingsCtrl.getUseCal () && avgBackground.size () == rawPicData.front ().size ()){
		for (auto picInc : range (rawPicData.size ())){
			calPicData[picInc] = Matrix<long> (rawPicData[picInc].getRows (), rawPicData[picInc].getCols (), 0);
			for (auto pixInc : range (rawPicData[picInc].size ()))
			{
				calPicData[picInc].data[pixInc] = (rawPicData[picInc].data[pixInc] - avgBackground.data[pixInc]);
			}
		}
	}
	else { calPicData = rawPicData; }

	if (picNum % 2 == 1){
		imageGrabTimes.push_back (std::chrono::high_resolution_clock::now ());
	}
	size_t currentActivePicNum = curSettings.continuousMode ? 0 : picNum % curSettings.picsPerRepetition;
	//emit newImage({ {picNum, repVar.first, repVar.second}, calPicData[currentActivePicNum] });

	/// send picture data to plotter
	qDebug() << "send Image data for drawing for image " << picNum << " at time " << timerE.elapsed() << " ms";
	auto picsToDraw = andorSettingsCtrl.getImagesToDraw (calPicData);
	try
	{
		std::pair<int, int> minMax;
		// draw the most recent pic.
		minMax = stats.update (picsToDraw.back (), currentActivePicNum, selectedPixel,
			picNum / curSettings.picsPerRepetition,
			curSettings.totalPicsInExperiment () / curSettings.picsPerRepetition);
		QPainter painter (this);
		pics.drawBitmap (picsToDraw.back (), minMax, currentActivePicNum,
			analysisHandler.getRunningSettings ().grids, picNum, 
			analysisHandler.getRunningSettings ().displayGridOption, painter);
			
		timer.update(picNum / curSettings.picsPerRepetition, curSettings.repetitionsPerVariation,
			curSettings.totalVariations, curSettings.picsPerRepetition, curSettings.repFirst);
	}
	catch (ChimeraError& err){
		reportErr (qstr (err.trace ()));
		try {
			mainWin->pauseExperiment ();
		}
		catch (ChimeraError & err) {
			reportErr (qstr (err.trace ()));
		}
	}
	/// write the data to the file.
	qDebug() << "write image to file for image " << picNum << " at time " << timerE.elapsed() << " ms";
	if (true/*curSettings.acquisitionMode != AndorRunModes::mode::Video*/){
		try	{
			// important! write the original raw data, not the pic-to-draw, which can be a difference pic, or the calibrated
			// pictures, which can have the background subtracted.
			dataHandler.writeAndorPic ( rawPicData[currentActivePicNum],
									    curSettings.imageSettings );
		}
		catch (ChimeraError& err){
			reportErr (err.qtrace ());
			try {
				if (!mainWin->experimentIsPaused()) {
					mainWin->pauseExperiment();
				}
			}
			catch (ChimeraError & err2) {
				reportErr (err2.qtrace ());
			}
		}
	}
	mostRecentPicNum = picNum;
	qDebug() << "finish write image to file for image " << picNum << " at time " << timerE.elapsed() << " ms";
	if (picNum == curSettings.totalPicsInExperiment() - 1) {
		andor.onFinish();
	}
}

void QtAndorWindow::wakeRearranger (){
	std::unique_lock<std::mutex> lock (rearrangerLock);
	rearrangerConditionVariable.notify_all ();
}

LRESULT QtAndorWindow::onCameraCalFinish (WPARAM wParam, LPARAM lParam){
	// notify the andor object that it is done.
	andor.onFinish ();
	andor.pauseThread ();
	andor.setCalibrating (false);
	justCalibrated = true;
	andorSettingsCtrl.cameraIsOn (false);
	// normalize.
	for (auto& p : avgBackground){
		p /= 100.0;
	}
	// if auto cal is selected, always assume that the user was trying to start with F5.
	if (andorSettingsCtrl.getAutoCal ()){
		//PostMessageA (WM_COMMAND, MAKEWPARAM (ID_ACCELERATOR_F5, 0));
	}
	return 0;
}

dataPoint QtAndorWindow::getMainAnalysisResult (){
	return mostRecentAnalysisResult;
}

void QtAndorWindow::cleanUpAfterExp (){
	atomCrunchThreadActive = false;
	dataHandler.normalCloseFile ();
}

int QtAndorWindow::getMostRecentFid (){
	return dataHandler.getDataFileNumber ();
}

int QtAndorWindow::getPicsPerRep (){
	return andorSettingsCtrl.getConfigSettings ().andor.picsPerRepetition;
}

std::string QtAndorWindow::getMostRecentDateString (){
	return dataHandler.getMostRecentDateString ();
}

bool QtAndorWindow::wantsThresholdAnalysis (){
	return analysisHandler.getRunningSettings ().autoThresholdAnalysisOption;
}

atomGrid QtAndorWindow::getMainAtomGrid (){
	return analysisHandler.getRunningSettings ().grids[0];
}


void QtAndorWindow::armCameraWindow (AndorRunSettings* settings){
	if (!settings->continuousMode) {
		pics.setNumberPicturesActive(settings->picsPerRepetition);
		if (settings->picsPerRepetition == 1) {
			pics.setSinglePicture(settings->imageSettings);
		}
		else {
			pics.setMultiplePictures(settings->imageSettings, settings->picsPerRepetition);
		}
	}
	else {
		pics.setSinglePicture(settings->imageSettings);
	}
	pics.resetPictureStorage ();
	pics.setParameters (settings->imageSettings);
	redrawPictures (false);
	andorSettingsCtrl.setRunSettings (*settings);
	andorSettingsCtrl.setRepsPerVariation (settings->repetitionsPerVariation);
	andorSettingsCtrl.setVariationNumber (settings->totalVariations);
	pics.setSoftwareAccumulationOptions (andorSettingsCtrl.getSoftwareAccumulationOptions ());
	try {
		andor.preparationChecks ();
	}
	catch (ChimeraError & err) {
		reportErr (err.qtrace ());
	}
	// turn some buttons off.
	andorSettingsCtrl.cameraIsOn (true);
	stats.reset ();
	analysisHandler.updateDataSetNumberEdit (dataHandler.getNextFileNumber () - 1);
}

bool QtAndorWindow::getCameraStatus (){
	return andor.isRunning ();
}

void QtAndorWindow::stopSound (){
	alerts.stopSound ();
}

void QtAndorWindow::passSetTemperaturePress (){
	try{
		if (andor.isRunning ()){
			thrower ("ERROR: the camera (thinks that it?) is running. You can't change temperature settings during camera "
				"operation.");
		}
		andorSettingsCtrl.handleSetTemperaturePress ();
		auto settings = andorSettingsCtrl.getConfigSettings ();
		andor.setSettings (settings.andor);
		andor.setTemperature ();
	}
	catch (ChimeraError& err){
		reportErr (qstr (err.trace ()));
	}
	mainWin->updateConfigurationSavedStatus (false);
}

void QtAndorWindow::assertDataFileClosed () {
	dataHandler.assertClosed ();
}

void QtAndorWindow::handlePictureSettings (){
	selectedPixel = { 0,0 };
	const unsigned picsPerRep = andorSettingsCtrl.getConfigSettings().andor.picsPerRepetition;
	const imageParameters imageSettings = andorSettingsCtrl.getConfigSettings().andor.imageSettings;
	const bool continuousMode = andorSettingsCtrl.getConfigSettings().andor.continuousMode;
	const AndorTriggerMode::mode triggerMode = andorSettingsCtrl.getConfigSettings().andor.triggerMode;
	if (continuousMode && (triggerMode!= AndorTriggerMode::mode::ExternalExposure)) {
		errBox("The continuous mode is checked and can only support External Exposure as trigger mode, that is, the exposure time number will be ignored"
			" and only the exposure time is determined by the on-time of the TTL trigger.");
		return;
	}

	andorSettingsCtrl.handlePictureSettings ();
	if (!continuousMode) {
		if (picsPerRep == 1) {
			pics.setSinglePicture(imageSettings);
		}
		else {
			pics.setMultiplePictures(imageSettings, picsPerRep);
		}
	}
	else {
		pics.setSinglePicture(imageSettings);
	}
	
	pics.resetPictureStorage ();
	std::array<int, 4> nums = andorSettingsCtrl.getConfigSettings ().palleteNumbers;
	pics.setPalletes (nums);
	analysisHandler.updateUnofficialPicsPerRep (picsPerRep, continuousMode);
}

/*
Check that the camera is idle, or not aquiring pictures. Also checks that the data analysis handler isn't active.
*/
void QtAndorWindow::checkCameraIdle (){
	if (andor.isRunning ()){
		thrower ("Camera is already running! Please Abort to restart.\r\n");
	}
	// make sure it's idle.
	try{
		andor.queryStatus ();
		if (ANDOR_SAFEMODE){
			thrower ("DRV_IDLE");
		}
	}
	catch (ChimeraError& exception){
		if (exception.whatBare () != "DRV_IDLE"){
			throwNested (" while querying andor status to check if idle.");
		}
	}
}

void QtAndorWindow::handleMasterConfigSave (std::stringstream& configStream){
	andorSettingsCtrl.handelSaveMasterConfig (configStream);
}

void QtAndorWindow::handleMasterConfigOpen (ConfigStream& configStream){
	mainWin->updateConfigurationSavedStatus (false);
	selectedPixel = { 0,0 };
	andorSettingsCtrl.handleOpenMasterConfig (configStream, this);
	pics.setParameters (andorSettingsCtrl.getConfigSettings ().andor.imageSettings);
	redrawPictures (true);
}

DataLogger& QtAndorWindow::getLogger (){
	return dataHandler;
}

void QtAndorWindow::loadCameraCalSettings (AllExperimentInput& input){
	redrawPictures (false);
	try{
		checkCameraIdle ();
	}
	catch (ChimeraError& err){
		reportErr (qstr (err.trace ()));
	}
	// I used to mandate use of a button to change image parameters. Now I don't have the button and just always 
	// update at this point.
	readImageParameters ();
	pics.setNumberPicturesActive (1);
	// biggest check here, camera settings includes a lot of things.
	andorSettingsCtrl.checkIfReady ();
	// reset the image which is about to be calibrated.
	avgBackground = Matrix<long> (0, 0);
	/// start the camera.
	andor.setCalibrating (true);
}

AndorCameraCore& QtAndorWindow::getCamera (){
	return andor;
}


void QtAndorWindow::prepareAtomCruncher (AllExperimentInput& input){
	input.cruncherInput = new atomCruncherInput;
	//input.cruncherInput->plotterActive = plotThreadActive;
	input.cruncherInput->imageDims = andorSettingsCtrl.getRunningSettings().imageSettings;
	atomCrunchThreadActive = true;
	//input.cruncherInput->plotterNeedsImages = input.masterInput->plotterInput->needsCounts;
	input.cruncherInput->cruncherThreadActive = &atomCrunchThreadActive;
	skipNext = false;
	input.cruncherInput->skipNext = &skipNext;
	//input.cruncherInput->imQueue = &imQueue;
	// options
	if (input.masterInput){
		input.cruncherInput->rearrangerActive = false;
	}
	else{
		input.cruncherInput->rearrangerActive = false;
	}
	input.cruncherInput->grids = analysisHandler.getRunningSettings ().grids;
	input.cruncherInput->thresholds = andorSettingsCtrl.getConfigSettings ().thresholds;
	input.cruncherInput->picsPerRep = andorSettingsCtrl.getRunningSettings ().picsPerRepetition;
	input.cruncherInput->catchPicTime = &crunchSeesTimes;
	input.cruncherInput->finTime = &crunchFinTimes;
	input.cruncherInput->atomThresholdForSkip = mainWin->getMainOptions ().atomSkipThreshold;
	input.cruncherInput->rearrangerConditionWatcher = &rearrangerConditionVariable;
}

bool QtAndorWindow::wantsAutoPause (){
	return alerts.wantsAutoPause ();
}

void QtAndorWindow::completeCruncherStart () {
	if ((mainWin->getExpThread() == nullptr) || (mainWin->getExpThread()->isFinished())) {
		// then this is called from ProgramNow in AndorWindow
		return;
	}
	auto cruncherInput = std::make_unique<atomCruncherInput>();
	cruncherInput->imageQueue = andor.getGrabberQueue();
	cruncherInput->imageDims = andorSettingsCtrl.getRunningSettings().imageSettings;
	cruncherInput->andorContinuousMode = andorSettingsCtrl.getRunningSettings ().continuousMode;
	atomCrunchThreadActive = true;
	cruncherInput->cruncherThreadActive = &atomCrunchThreadActive;
	skipNext = false;
	cruncherInput->skipNext = &skipNext;
	cruncherInput->rearrangerActive = false;
	cruncherInput->grids = analysisHandler.getRunningSettings ().grids;
	cruncherInput->thresholds = andorSettingsCtrl.getConfigSettings ().thresholds;
	cruncherInput->picsPerRep = andorSettingsCtrl.getRunningSettings ().picsPerRepetition;
	cruncherInput->catchPicTime = &crunchSeesTimes;
	cruncherInput->finTime = &crunchFinTimes;
	cruncherInput->atomThresholdForSkip = mainWin->getMainOptions ().atomSkipThreshold;
	cruncherInput->rearrangerConditionWatcher = &rearrangerConditionVariable;

	atomCruncherWorker = new CruncherThreadWorker(std::move(cruncherInput));
	QThread* thread = new QThread;
	atomCruncherWorker->moveToThread(thread);
	connect(thread, &QThread::started, atomCruncherWorker, &CruncherThreadWorker::init);
	connect(mainWin->getExpThread(), &QThread::finished, thread, &QThread::quit);
	connect(thread, &QThread::finished, atomCruncherWorker, &CruncherThreadWorker::deleteLater);
	connect(atomCruncherWorker, &QThread::destroyed, thread, &CruncherThreadWorker::deleteLater);
	//connect(this, &QtAndorWindow::newImage, atomCruncherWorker, &CruncherThreadWorker::handleImage);
	thread->start();
}

void QtAndorWindow::completePlotterStart () {
	/// start the plotting thread.
	auto pltInput = std::make_unique<realTimePlotterInput>();
	pltInput->plotParentWindow = this;
	
	auto camSettings = andorSettingsCtrl.getRunningSettings ();
	pltInput->variations = camSettings.totalVariations;
	pltInput->picsPerVariation = camSettings.totalPicsInVariation();

	pltInput->imageShape = camSettings.imageSettings;
	pltInput->picsPerRep = camSettings.picsPerRepetition;
	
	pltInput->alertThreshold = alerts.getAlertThreshold ();
	pltInput->wantAtomAlerts = alerts.wantsAtomAlerts ();
	analysisHandler.fillPlotThreadInput (pltInput.get());
	// remove old plots that aren't trying to sustain.
	unsigned mainPlotInc = 0;
	for (auto plotParams : pltInput->plotInfo) {
		plotStyle style = plotParams.isHist ? plotStyle::HistPlot : plotStyle::BinomialDataPlot;
		if (mainPlotInc < 6) {
			mainAnalysisPlots[mainPlotInc]->setStyle (style);
			mainAnalysisPlots[mainPlotInc]->setThresholds (andorSettingsCtrl.getConfigSettings ().thresholds[0]);
			mainAnalysisPlots[mainPlotInc]->setTitle (plotParams.name);
			mainPlotInc++;
		}
	}

	bool gridHasBeenSet = false;
	for (auto gridInfo : pltInput->grids) {
		if (!(gridInfo.gridOrigin == coordinate(0, 0)) || gridInfo.useFile) {
			gridHasBeenSet = true;
			break;
		}
	}
	if ((!gridHasBeenSet) || pltInput->plotInfo.size () == 0) {
		//plotThreadActive = false;
	}
	else if (andorSettingsCtrl.getConfigSettings().andor.continuousMode) {
		//plotThreadActive = false;
	}
	else {
		// start the plotting thread
		analysisThreadWorker = new AnalysisThreadWorker (std::move(pltInput));
		QThread* thread = new QThread;
		analysisThreadWorker->moveToThread (thread);
		connect (thread, &QThread::started, analysisThreadWorker, &AnalysisThreadWorker::init);
		connect(mainWin->getExpThread(), &QThread::finished, thread, &QThread::quit);
		connect(thread, &QThread::finished, analysisThreadWorker, &AnalysisThreadWorker::deleteLater);
		connect(analysisThreadWorker, &AnalysisThreadWorker::destroyed, thread, &QThread::deleteLater);

		connect (mainWin->getExpThreadWorker(), &ExpThreadWorker::plot_Xvals_determined,
				 analysisThreadWorker, &AnalysisThreadWorker::setXpts);
		connect (analysisThreadWorker, &AnalysisThreadWorker::newPlotData, this,
			[this](std::vector<std::vector<dataPoint>> data, int plotNum) {mainAnalysisPlots[plotNum]->setData (data); });
		if (atomCruncherWorker) {
			connect (atomCruncherWorker, &CruncherThreadWorker::atomArray,
				analysisThreadWorker, &AnalysisThreadWorker::handleNewPic);
			connect (atomCruncherWorker, &CruncherThreadWorker::pixArray,
				analysisThreadWorker, &AnalysisThreadWorker::handleNewPix);
		}
		thread->start ();
	}
}

bool QtAndorWindow::wantsNoMotAlert (){
	if (cameraIsRunning ()){
		return alerts.wantsMotAlerts ();
	}
	else{
		return false;
	}
}

unsigned QtAndorWindow::getNoMotThreshold (){
	return alerts.getAlertThreshold ();
}

std::string QtAndorWindow::getStartMessage (){
	// get selected plots
	auto andrSttngs = andorSettingsCtrl.getConfigSettings ().andor;
	std::vector<std::string> plots = analysisHandler.getActivePlotList ();
	imageParameters currentImageParameters = andrSttngs.imageSettings;
	bool errCheck = false;
	for (unsigned plotInc = 0; plotInc < plots.size (); plotInc++){
		PlottingInfo tempInfoCheck (PLOT_FILES_SAVE_LOCATION + "\\" + plots[plotInc] + ".plot");
		if ((!andrSttngs.continuousMode) && (tempInfoCheck.getPicNumber() != andrSttngs.picsPerRepetition)) {
			thrower (": one of the plots selected, " + plots[plotInc] + ", is not built for the currently "
					 "selected number of pictures per experiment. (" + str(andrSttngs.picsPerRepetition) 
					 + ") Please revise either the current setting or the plot file.");
		}
	}
	std::string dialogMsg;
	dialogMsg = "Camera Parameters:\r\n%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%\r\n";
	dialogMsg += "Current Camera Temperature Setting:\r\n\t" + str (
		andrSttngs.temperatureSetting) + "\r\n";
	dialogMsg += "Exposure Times: ";
	for (auto& time : andrSttngs.exposureTimes){
		dialogMsg += str (time * 1000) + ", ";
	}
	dialogMsg += "\r\n";
	dialogMsg += "Image Settings:\r\n\t" + str (currentImageParameters.left) + " - " + str (currentImageParameters.right) + ", "
		+ str (currentImageParameters.bottom) + " - " + str (currentImageParameters.top) + "\r\n";
	dialogMsg += "\r\n";
	dialogMsg += "FrameRate:\r\n\t" + str (andrSttngs.frameRate) + "\r\n";
	//dialogMsg += "Kintetic Cycle Time:\r\n\t" + str (andrSttngs.kineticCycleTime) + "\r\n";
	dialogMsg += "Pictures per Repetition:\r\n\t" + str (andrSttngs.picsPerRepetition) + "\r\n";
	dialogMsg += "Repetitions per Variation:\r\n\t" + str (andrSttngs.totalPicsInVariation ()) + "\r\n";
	dialogMsg += "Variations per Experiment:\r\n\t" + str (andrSttngs.totalVariations) + "\r\n";
	dialogMsg += "Total Pictures per Experiment:\r\n\t" + str (andrSttngs.totalPicsInExperiment ()) + "\r\n";

	dialogMsg += "Real-Time Atom Detection Thresholds:\r\n\t";
	unsigned count = 0;
	for (auto& picThresholds : andorSettingsCtrl.getConfigSettings ().thresholds){
		dialogMsg += "Pic " + str (count) + " thresholds: ";
		for (auto thresh : picThresholds){
			dialogMsg += str (thresh) + ", ";
		}
		dialogMsg += "\r\n";
		count++;
	}
	dialogMsg += "\r\nReal-Time Plots:\r\n";
	for (unsigned plotInc = 0; plotInc < plots.size (); plotInc++){
		dialogMsg += "\t" + plots[plotInc] + "\r\n";
	}
	return dialogMsg;
}

void QtAndorWindow::fillMasterThreadInput (ExperimentThreadInput* input){
	// starting a not-calibration, so reset this.
	justCalibrated = false;
	input->rearrangerLock = &rearrangerLock;
	input->andorsImageTimes = &imageTimes;
	input->grabTimes = &imageGrabTimes;
	input->conditionVariableForRerng = &rearrangerConditionVariable;
}

void QtAndorWindow::setTimerText (std::string timerText){
	timer.setTimerDisplay (timerText);
}

void QtAndorWindow::setDataType (std::string dataType){
	stats.updateType (dataType);
}

void QtAndorWindow::redrawPictures (bool andGrid){
	try	{
		if (andGrid){
			QPainter painter (this);
			pics.drawGrids (painter);
		}
		// ??? should there be handling here???
	}
	catch (ChimeraError& err){
		reportErr (err.qtrace ());
	}
	// currently don't attempt to redraw previous picture data.
}

std::atomic<bool>* QtAndorWindow::getSkipNextAtomic (){
	return &skipNext;
}

// this is typically a little redundant to call, but can use to make sure things are set to off.
void QtAndorWindow::assertOff (){
	andorSettingsCtrl.cameraIsOn (false);
	atomCrunchThreadActive = false;
}

void QtAndorWindow::readImageParameters (){
	selectedPixel = { 0,0 };
	try	{
		redrawPictures (false);
		imageParameters parameters = andorSettingsCtrl.getConfigSettings ().andor.imageSettings;
		pics.setParameters (parameters);
	}
	catch (ChimeraError& exception){
		reportErr (exception.qtrace () + "\r\n");
	}
	QPainter painter (this);
	pics.drawGrids (painter);
}

void QtAndorWindow::fillExpDeviceList (DeviceList& list){
	list.list.push_back (andor);
}

void QtAndorWindow::handleNormalFinish (profileSettings finishedProfile) {
	wakeRearranger ();
	cleanUpAfterExp ();
	handleBumpAnalysis (finishedProfile);
}

void QtAndorWindow::handleBumpAnalysis (profileSettings finishedProfile) {
	std::ifstream configFileRaw (finishedProfile.configFilePath ());
	// check if opened correctly.
	if (!configFileRaw.is_open ()) {
		errBox ("Opening of Configuration File for bump analysis Failed!");
		return;
	}
	ConfigStream cStream (configFileRaw);
	cStream.setCase (false);
	configFileRaw.close ();
	ConfigSystem::getVersionFromFile (cStream);
	ConfigSystem::jumpToDelimiter (cStream, "DATA_ANALYSIS");
	auto settings = analysisHandler.getAnalysisSettingsFromFile (cStream);
	// get the options from the config file, not from the current config settings. this is important especially for 
	// handling this in the calibration. 
	if (settings.autoBumpOption) {
		auto grid = andorWin->getMainAtomGrid ();
		auto dateStr = andorWin->getMostRecentDateString ();
		auto fid = andorWin->getMostRecentFid ();
		auto ppr = andorWin->getPicsPerRep ();
		try {
			auto res = pythonHandler.runCarrierAnalysis (dateStr, fid, grid, this);
			auto name =	settings.bumpParam;
			// zero is the default.
			if (name != "" && res != 0) {
				auxWin->getGlobals ().adjustVariableValue (str (name, 13, false, true), res);
				// TODO adapt this for cryo. ZZP 06/26/2021
			}
			reportStatus ( qstr("Successfully completed auto bump analysis and set variable \"" + name + "\" to value " 
						   + str (res) + "\n"));
		}
		catch (ChimeraError & err) {
			reportErr ("Bump Analysis Failed! " + err.qtrace ());
		}
	}
}

NewPythonHandler* QtAndorWindow::getPython() {
	return &pythonHandler;
}

void QtAndorWindow::manualProgramCameraSetting()
{
}

