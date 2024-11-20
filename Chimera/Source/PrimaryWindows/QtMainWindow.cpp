#include "stdafx.h"
#include "QtMainWindow.h"
#include <ExperimentMonitoringAndStatus/ColorBox.h>
#include <qdesktopwidget.h>
#include <PrimaryWindows/QtScriptWindow.h>
#include <PrimaryWindows/QtAndorWindow.h>
#include <PrimaryWindows/QtAuxiliaryWindow.h>
#include <PrimaryWindows/QtMakoWindow.h>
#include <PrimaryWindows/QtAnalysisWindow.h>
#include <ExperimentThread/autoCalConfigInfo.h>
#include <GeneralObjects/ChimeraStyleSheets.h>
#include <ExperimentThread/ExpThreadWorker.h>
#include <QThread.h>
#include <qapplication.h>
#include <qwidget.h>
#include <qwindow.h>
#include <qscreen.h>

QtMainWindow::QtMainWindow () : 
	profile (PROFILES_PATH, this),
	masterConfig (MASTER_CONFIGURATION_FILE_ADDRESS),
	tempMonitor(this, TEMPMON_SAFEMODE),
	tcpServer(this)
{
	
	startupTimes.push_back (chronoClock::now ());
	/// Initialize Windows
	std::string which = "";
	try	{
		mainWin = this;
		which = "Scripting";
		scriptWin = new QtScriptWindow;
		which = "Camera";
		andorWin = new QtAndorWindow;
		which = "Auxiliary";
		auxWin = new QtAuxiliaryWindow;
		which = "CMOS";
		makoWin1 = new QtMakoWindow(1, MakoInfo::camWindow1);
		which = "CMOS";
		makoWin2 = new QtMakoWindow(2, MakoInfo::camWindow2);
		which = "Analysis";
		analysisWin = new QtAnalysisWindow;
	}
	catch (ChimeraError& err) {
		errBox ("FATAL ERROR: " + which + " Window constructor failed! Error: " + err.trace ());
		return;
	}
	scriptWin->loadFriends( this, scriptWin, auxWin, andorWin, makoWin1, makoWin2, analysisWin);
	andorWin->loadFriends (this, scriptWin, auxWin, andorWin, makoWin1, makoWin2, analysisWin);
	auxWin->loadFriends (this, scriptWin, auxWin, andorWin, makoWin1, makoWin2, analysisWin);
	makoWin1->loadFriends(this, scriptWin, auxWin, andorWin, makoWin1, makoWin2, analysisWin);
	makoWin2->loadFriends(this, scriptWin, auxWin, andorWin, makoWin1, makoWin2, analysisWin);
	analysisWin->loadFriends(this, scriptWin, auxWin, andorWin, makoWin1, makoWin2, analysisWin);
	startupTimes.push_back (chronoClock::now ());

	for (auto* window : winList ()) {
		window->initializeWidgets ();
		window->initializeShortcuts ();
		window->initializeMenu ();
	}
	setStyleSheets ();
	auto numMonitors = qApp->screens ().size ();
	auto screens = qApp->screens ();
	unsigned winCount = 0;
	std::vector<unsigned> monitorNum = { 1,0,3,0,0,3,2 };
	/*	scriptWin, andorWin, auxWin, makoWin1, makoWin2, analysisWin, mainWin; */
	for (auto* window : winList ()) { 
		auto screen = qApp->screens ()[monitorNum[winCount++] % numMonitors];
		window->setWindowState ((windowState () & ~Qt::WindowMinimized) | Qt::WindowActive);
		window->activateWindow ();
		//window->setGeometry(0, 0, 1200, 800);
		window->showMaximized (); 
		window->move (screen->availableGeometry ().topLeft());
		//window->resize (screen->availableGeometry ().width (), screen->availableGeometry().height());
	}
	andorWin->activateWindow(); // bring to front
	auxWin->activateWindow();
	andorWin->refreshPics();
	andorWin->refreshPics();
	// hide the splash just before the first window requiring input pops up.
	try	{
		masterConfig.load (this, auxWin, andorWin);
	}
	catch (ChimeraError& err){
		errBox (err.trace ());
	}
	setWindowTitle ("Main Window");
	updateConfigurationSavedStatus (true);

	/// summarize system status.
	try {
		// ordering of aux window pieces is a bit funny because I want the devices grouped by type, not by window.
		std::string initializationString;
		initializationString += getSystemStatusString ();
		initializationString += auxWin->getOtherSystemStatusMsg ();
		initializationString += andorWin->getSystemStatusString ();
		initializationString += auxWin->getVisaDeviceStatus ();
		initializationString += makoWin1->getSystemStatusString();
		initializationString += makoWin2->getSystemStatusString();
		initializationString += scriptWin->getSystemStatusString ();
		initializationString += analysisWin->getSystemStatusString();
		reportStatus (qstr(initializationString));
	}
	catch (ChimeraError & err) {
		errBox (err.trace ());
	}
	
	QTimer* timer = new QTimer (this);
	connect (timer, &QTimer::timeout, [this]() {
		// should auto quit in the handling here if calibration has already been completed for the day. 
		commonFunctions::handleCommonMessage (ID_ACCELERATOR_F11, this);
		});
	// 6 minutes
	timer->start (360000);

}

bool QtMainWindow::expIsRunning () {
	return experimentIsRunning;
}

void QtMainWindow::setStyleSheets (){
	for (auto* window : winList ()) {
		window->setStyleSheet (chimeraStyleSheets::stdStyleSheet ());
	}
}

void QtMainWindow::pauseExperiment () {
	if (expWorker != nullptr) {
		expWorker->pause ();
	}
}

void QtMainWindow::initializeWidgets (){
	statBox = new ColorBox(this, getDevices());

	/// initialize main window controls.
	QWidget* centralWidget = new QWidget();
	setCentralWidget(centralWidget);
	QGridLayout* layout = new QGridLayout(centralWidget);
	//centralWidget->setStyleSheet("border: 2px solid  black; ");

	mainStatus.initialize (this, "EXPERIMENT STATUS", { "#7474FF","#4848FF","#2222EF" });
	shortStatus.initialize (this);
	errorStatus.initialize (this, "ERROR STATUS", { "#FF0000", "#800000"});
	
	layout->addWidget(&mainStatus, 0, 0, 6, 1);
	layout->addWidget(shortStatus.statusLabel(), 6, 0, 1, 2);
	layout->addWidget(&errorStatus, 0, 1, 6, 1);

	profile.initialize (this); // this and inside it, "handleSelectConfigButton" connect the open config button to openning the config for all windows 
	notes.initialize (this);
	tcpServer.initialize();
	repetitionControl.initialize (this);
	mainOptsCtrl.initialize (this);
	debugger.initialize (this);
	tempMonitor.initialize(this);

	layout->addWidget(&profile, 0, 2);
	layout->addWidget(&notes, 1, 2);
	layout->addWidget(&tcpServer, 2, 2);
	layout->addWidget(&repetitionControl, 3, 2);
	layout->addWidget(&mainOptsCtrl, 4, 2);
	layout->addWidget(&debugger, 5, 2);
	layout->addWidget(&tempMonitor, 6, 2);
}

unsigned QtMainWindow::getAutoCalNumber () { return autoCalNum; }

void QtMainWindow::onAutoCalFin (QString msg, profileSettings finishedConfig){
	andorWin->handleNormalFinish (finishedConfig);
	autoCalNum++;
	if (autoCalNum >= AUTO_CAL_LIST.size ())	{
		// then just finished the calibrations.
		autoCalNum = 0;
		infoBox ("Finished Automatic Calibrations.");
	}
	else{
		commonFunctions::handleCommonMessage (ID_ACCELERATOR_F11, this);
	}
}

void QtMainWindow::loadCameraCalSettings (ExperimentThreadInput* input){
	input->skipNext = nullptr;
	input->expType = ExperimentType::CameraCal;
}

void QtMainWindow::showHardwareStatus (){
	try	{
		// ordering of aux window pieces is a bit funny because I want the devices grouped by type, not by window.
		std::string initializationString;
		initializationString += getSystemStatusString();
		initializationString += auxWin->getOtherSystemStatusMsg();
		initializationString += andorWin->getSystemStatusString();
		initializationString += auxWin->getVisaDeviceStatus();
		initializationString += makoWin1->getSystemStatusString();
		initializationString += makoWin2->getSystemStatusString();
		initializationString += scriptWin->getSystemStatusString();
		initializationString += analysisWin->getSystemStatusString();
		infoBox (initializationString);
	}
	catch (ChimeraError& err)	{
		reportErr (err.qtrace ());
	}
}

// just notifies the profile object that the configuration is no longer saved.
void QtMainWindow::notifyConfigUpdate (){
	profile.updateConfigurationSavedStatus (false);
}

void QtMainWindow::handlePauseToggle (){
	if (expWorker->runningStatus ()){
		reportErr ("Pause Toggle!");
		if (expWorker->getIsPaused ()){
			expWorker->unPause ();
		}
		else{
			expWorker->pause ();
		}
	}
	else{
		reportStatus ("Can't pause, experiment was not running.\r\n");
	}
}

void QtMainWindow::onRepProgress (unsigned int repNum){
	repetitionControl.updateNumber (repNum);
}

void QtMainWindow::windowSaveConfig (ConfigStream& saveFile){
	notes.handleSaveConfig (saveFile);
	mainOptsCtrl.handleSaveConfig (saveFile);
	debugger.handleSaveConfig (saveFile);
	repetitionControl.handleSaveConfig (saveFile);
}

void QtMainWindow::windowOpenConfig (ConfigStream& configStream){
	try	{
		ConfigSystem::standardOpenConfig (configStream, "CONFIGURATION_NOTES", &notes);
		mainOptsCtrl.setOptions (ConfigSystem::stdConfigGetter (configStream, "MAIN_OPTIONS",
			MainOptionsControl::getSettingsFromConfig));
		ConfigSystem::standardOpenConfig (configStream, "DEBUGGING_OPTIONS", &debugger);
		repetitionControl.setRepetitions (ConfigSystem::stdConfigGetter (configStream, "REPETITIONS",
			Repetitions::getSettingsFromConfig));
	}
	catch (ChimeraError&){
		throwNested ("Main Window failed to read parameters from the configuration file.");
	}
}

unsigned QtMainWindow::getRepNumber () { return repetitionControl.getRepetitionNumber (); }

std::string QtMainWindow::getSystemStatusString (){
	std::string status;
	return status;
}

void QtMainWindow::startExperimentThread (ExperimentThreadInput* input){
	//expThreadManager.startExperimentThread (input, this);
	if (!input) {
		thrower ("Input to start experiment thread was null?!?!? (a Low level bug, this shouldn't happen).");
	}
	if (experimentIsRunning) {
		delete input;
		thrower ("Experiment is already Running! You can only run one experiment at a time! Please abort before "
			"running again.");
	}
	//input->thisObj = this;
	expWorker = new ExpThreadWorker (input, experimentIsRunning);
	expThread = new QThread;
	expWorker->moveToThread (expThread);
	connect (expWorker, &ExpThreadWorker::updateBoxColor, this, &QtMainWindow::handleColorboxUpdate);
	connect (expWorker, &ExpThreadWorker::prepareAndor, andorWin, &QtAndorWindow::handlePrepareForAcq, Qt::BlockingQueuedConnection);
	connect (expWorker, &ExpThreadWorker::prepareMako, makoWin1, &QtMakoWindow::prepareWinForAcq, Qt::BlockingQueuedConnection);// want expthread wait untill mako set it up
	connect (expWorker, &ExpThreadWorker::prepareMako, makoWin2, &QtMakoWindow::prepareWinForAcq, Qt::BlockingQueuedConnection);// want expthread wait untill mako set it up
	connect (expWorker, &ExpThreadWorker::prepareAnalysis, analysisWin, &QtAnalysisWindow::prepareCalcForAcq);
	connect (expWorker, &ExpThreadWorker::notification, this, &QtMainWindow::handleNotification);
	connect (expWorker, &ExpThreadWorker::warn, this, &QtMainWindow::onErrorMessage);
	//connect (expWorker, &ExpThreadWorker::doAoData, auxWin, &QtAuxiliaryWindow::handleDoAoPlotData);
	connect (expWorker, &ExpThreadWorker::doAoOlData, &analysisWin->SeqPlotter, &ExperimentSeqPlotter::handleDoAoOlPlotData);
	connect (expWorker, &ExpThreadWorker::repUpdate, this, &QtMainWindow::onRepProgress);
	connect (expWorker, &ExpThreadWorker::mainProcessFinish, expThread, &QThread::quit);
	connect (expWorker, &ExpThreadWorker::normalExperimentFinish, this, &QtMainWindow::onNormalFinish);
	connect (expWorker, &ExpThreadWorker::calibrationFinish, this, &QtMainWindow::onAutoCalFin);
	connect (expWorker, &ExpThreadWorker::errorExperimentFinish, this, &QtMainWindow::onFatalError);
	connect (expWorker, &ExpThreadWorker::expParamsSet, this->auxWin, &QtAuxiliaryWindow::updateExpActiveInfo);
	connect(expWorker, &ExpThreadWorker::expCalibrationsSet, this->auxWin, &QtAuxiliaryWindow::updateCalActiveInfo, Qt::BlockingQueuedConnection); // want expthread wait untill cal is set
	connect(expWorker, &ExpThreadWorker::startInExpCalibrationTimer, &(this->mainOptsCtrl), &MainOptionsControl::startInExpCalibrationTimer, Qt::BlockingQueuedConnection);

	connect (expThread, &QThread::started, expWorker, &ExpThreadWorker::process);
	connect (expThread, &QThread::finished, expWorker, &QObject::deleteLater);
	connect (expWorker, &QObject::destroyed, expThread, &QObject::deleteLater);
	expThread->start (QThread::TimeCriticalPriority);
}


void QtMainWindow::fillMotInput (ExperimentThreadInput* input){
	input->profile.configuration = "Set MOT Settings";
	input->profile.configLocation = MOT_ROUTINES_ADDRESS;
	input->profile.parentFolderName = "MOT";
	input->skipNext = nullptr;
}

bool QtMainWindow::masterIsRunning () { return experimentIsRunning; }
RunInfo QtMainWindow::getRunInfo() { return systemRunningInfo; }
profileSettings QtMainWindow::getProfileSettings () { return profile.getProfileSettings (); }
std::string QtMainWindow::getNotes () { return notes.getConfigurationNotes (); }
void QtMainWindow::setNotes (std::string newNotes) { notes.setConfigurationNotes (newNotes); }
debugInfo QtMainWindow::getDebuggingOptions () { return debugger.getOptions (); }
void QtMainWindow::setDebuggingOptions (debugInfo options) { debugger.setOptions (options); }
mainOptions QtMainWindow::getMainOptions () { return mainOptsCtrl.getOptions (); }
void QtMainWindow::setShortStatus (std::string text) { shortStatus.setText (text); }
void QtMainWindow::changeShortStatusColor (std::string color) { shortStatus.setColor (color); }
bool QtMainWindow::experimentIsPaused () { return expWorker->getIsPaused (); }

void QtMainWindow::fillMasterThreadInput (ExperimentThreadInput* input){
	input->sleepTime = debugger.getOptions ().sleepTime;
	input->profile = profile.getProfileSettings ();
	input->calInterrupt = mainOptsCtrl.interruptPointer();
}

void QtMainWindow::logParams (DataLogger* logger, ExperimentThreadInput* input){
	logger->logMasterInput (input);
}

void QtMainWindow::checkProfileSave (){
	profile.checkSaveEntireProfile( this );
}

void QtMainWindow::updateConfigurationSavedStatus (bool status){
	profile.updateConfigurationSavedStatus (status);
}

void QtMainWindow::addTimebar (std::string whichStatus){
	std::transform (whichStatus.begin (), whichStatus.end (), whichStatus.begin (), ::tolower);
	if (whichStatus == "error")	{
		errorStatus.appendTimebar ();
	}
	else if (whichStatus == "main")	{
		mainStatus.appendTimebar ();
	}
	else{
		thrower ("Main Window's addTimebar function recieved a bad argument for which status"
			" control to update. Options are \"error\", \"debug\", and \"main\", but recieved " + whichStatus + ". This"
			"exception can be safely ignored.");
	}
}

void QtMainWindow::changeBoxColor (std::string sysDelim, std::string color){
	IChimeraQtWindow::changeBoxColor (sysDelim, color);
	changeShortStatusColor (color);
}

void QtMainWindow::abortMasterThread (){
	if (expWorker->runningStatus ()){
		expWorker->abort ();
		autoF5_AfterFinish = false;
	}
	else { thrower ("Can't abort, experiment was not running.\r\n"); }
}

void QtMainWindow::onErrorMessage (QString errMessage, unsigned level){
	if (str(errMessage) != ""){
		QApplication::beep ();
		errorStatus.addStatusText (statusMsg( str(errMessage), level));
	}
}

void QtMainWindow::onFatalError (QString finMsg){
	onErrorMessage (finMsg);
	autoF5_AfterFinish = false;
	// resetting things.
	std::string msgText = "Exited with Error!\nPassively Outputting Default Waveform.";
	//andorWin->abortCameraRun(); this should be aborted in commonFunctions when one press Shift+F5, so no need to do it again
	auxWin->handleNormalFin();
	changeShortStatusColor ("R");
	reportErr ("EXITED WITH ERROR!\n");
	reportStatus ("EXITED WITH ERROR!\nInitialized Default Waveform\r\n");
}

void QtMainWindow::onNormalFinish (QString finMsg, profileSettings finishedProfile) {
	handleNotification (finMsg);
	setShortStatus ("Passively Outputting Default Waveform");
	changeShortStatusColor ("B");
	andorWin->handleNormalFinish (finishedProfile);
	handleFinishText ();
	auxWin->handleNormalFin ();
	if (autoF5_AfterFinish)	{
		commonFunctions::handleCommonMessage (ID_ACCELERATOR_F5, this);
		autoF5_AfterFinish = false;
	}
}

void QtMainWindow::handleFinishText (){
	time_t t = time (0);
	struct tm now;
	localtime_s (&now, &t);
	std::string message = "Experiment Completed at ";
	if (now.tm_hour < 10){
		message += "0";
	}
	message += str (now.tm_hour) + ":";
	if (now.tm_min < 10){
		message += "0";
	}
	message += str (now.tm_min) + ":";
	if (now.tm_sec < 10){
		message += "0";
	}
	message += str (now.tm_sec);
	try{
		//texter.sendMessage (message, &python, "Finished");
	}
	catch (ChimeraError& err){
		reportErr (err.qtrace ());
	}
}

void QtMainWindow::handleMasterConfigOpen (ConfigStream& configStream){
}

void QtMainWindow::handleMasterConfigSave (std::stringstream& configStream){
}

void QtMainWindow::fillExpDeviceList (DeviceList& list) {
	list.list.push_back(tempMonitor.getCore());
}

DeviceList QtMainWindow::getDevices (){
	DeviceList list;
	for (auto win_ : winList ()) {
		win_->fillExpDeviceList (list);
	}
	return list;
}

void QtMainWindow::handleColorboxUpdate (QString color, QString systemDelim){
	auto colorstr = str (color);
	auto delimStr = str (systemDelim);
	for (auto win_ : winList()) {
		if (win_ != nullptr) {
			win_->changeBoxColor(delimStr, colorstr);
		}
	}
}

void QtMainWindow::handleNotification (QString txt, unsigned level){
	mainStatus.addStatusText (statusMsg( str(txt), level ));
}

QThread* QtMainWindow::getExpThread () {
	return expThread;
}

ExpThreadWorker* QtMainWindow::getExpThreadWorker () {
	return expWorker;
}
