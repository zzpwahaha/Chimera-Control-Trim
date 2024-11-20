#include "stdafx.h"
#include "QtScriptWindow.h"
#include <qdesktopwidget.h>
#include <qlayout.h>
#include <PrimaryWindows/QtScriptWindow.h>
#include <PrimaryWindows/QtAndorWindow.h>
#include <PrimaryWindows/QtAuxiliaryWindow.h>
#include <PrimaryWindows/QtMakoWindow.h>
#include <PrimaryWindows/QtMainWindow.h>
#include <ExcessDialogs/saveWithExplorer.h>
#include <ExcessDialogs/openWithExplorer.h>

QtScriptWindow::QtScriptWindow(QWidget* parent) : IChimeraQtWindow(parent)
	, masterScript(this)
	, arbGens{ {ArbGenSystem(UWAVE_SIGLENT_SETTINGS, ArbGenType::Siglent, this),
		ArbGenSystem(UWAVE_AGILENT_SETTINGS, ArbGenType::Agilent, this) } }
	, gigaMoog(this)
{
	setWindowTitle ("Script Window");
}

QtScriptWindow::~QtScriptWindow (){
}

void QtScriptWindow::initializeWidgets (){
	statBox = new ColorBox(this, mainWin->getDevices());
	QWidget* centralWidget = new QWidget();
	setCentralWidget(centralWidget);
	QHBoxLayout* layout = new QHBoxLayout(centralWidget);
	//centralWidget->setStyleSheet("border: 2px solid  black; ");
	for (auto name : ArbGenEnum::allAgs) {
		arbGens[(int)name].initialize(arbGens[(int)name].initSettings.deviceName, this);
	}

	

	masterScript.initialize(this, "Master", "Master Script");
	gigaMoog.initialize(this);
	//profileDisplay.initialize (this);
	QVBoxLayout* layout1 = new QVBoxLayout(this);
	layout1->setContentsMargins(0, 0, 0, 0);
	layout1->addWidget(&arbGens[0], 1);
	layout1->addWidget(&arbGens[1], 1);
	layout->addLayout(layout1, 1);
	layout->addWidget(&gigaMoog, 1);
	layout->addWidget(&masterScript, 1);
	
	try {
		for (auto name : ArbGenEnum::allAgs) {
			arbGens[(int)name].setDefault(1);
			arbGens[(int)name].setDefault(2);
		}
		//intensityAgilent.setDefault(1);
	}
	catch (ChimeraError& err) {
		errBox("ERROR: Failed to initialize ArbGens: " + err.trace());
	}


	updateDoAoDdsNames ();
	updateVarNames ();
}

void QtScriptWindow::updateVarNames() {
	auto params = auxWin->getAllParams ();
	masterScript.highlighter->setGlobalParams(auxWin->getGlobalParams());
	masterScript.highlighter->setOtherParams (auxWin->getConfigParams());
	masterScript.highlighter->setLocalParams (masterScript.getLocalParams ());
	masterScript.highlighter->rehighlight();

	for (auto name : ArbGenEnum::allAgs) {
		arbGens[(int)name].arbGenScript.highlighter->setOtherParams(params);
		arbGens[(int)name].arbGenScript.highlighter->setLocalParams(arbGens[(int)name].arbGenScript.getLocalParams());
		arbGens[(int)name].arbGenScript.highlighter->rehighlight();
	}
}

void QtScriptWindow::updateDoAoDdsNames () {
	auto doNamesArr = auxWin->getTtlNames ();
	auto doNames = std::vector<std::string>(doNamesArr.begin(), doNamesArr.end());
	auto aoNamesArr = auxWin->getDacNames();
	auto aoNames = std::vector<std::string>(aoNamesArr.begin(), aoNamesArr.end());
	auto ddsNamesArr = auxWin->getDdsNames();
	auto ddsNames = std::vector<std::string>(ddsNamesArr.begin(), ddsNamesArr.end());
	auto olNamesArr = auxWin->getOlNames();
	auto olNames = std::vector<std::string>(olNamesArr.begin(), olNamesArr.end());
	auto calNames = auxWin->getCalNames();

	
	masterScript.highlighter->setTtlNames(doNames);
	masterScript.highlighter->setDacNames(aoNames);
	masterScript.highlighter->setDdsNames(ddsNames);
	masterScript.highlighter->setOlNames(olNames);
	masterScript.highlighter->setCalNames(calNames);
	masterScript.highlighter->rehighlight();

	for (auto name : ArbGenEnum::allAgs) {
		arbGens[(int)name].arbGenScript.highlighter->setTtlNames(doNames);
		arbGens[(int)name].arbGenScript.highlighter->setDacNames(aoNames);
		arbGens[(int)name].arbGenScript.highlighter->setDdsNames(ddsNames);
		arbGens[(int)name].arbGenScript.highlighter->setOlNames(olNames);
		arbGens[(int)name].arbGenScript.highlighter->setCalNames(calNames);
		arbGens[(int)name].arbGenScript.highlighter->rehighlight();
	}

}


void QtScriptWindow::handleMasterFunctionChange (){
	try{
		masterScript.functionChangeHandler (mainWin->getProfileSettings ().configLocation);
		masterScript.updateSavedStatus (true);
	}
	catch (ChimeraError& err){
		errBox (err.trace ());
	}
}

void QtScriptWindow::checkScriptSaves (){
	masterScript.checkSave (getProfile ().configLocation, mainWin->getRunInfo());
	gigaMoog.gmoogScript.checkSave(getProfile().configLocation, mainWin->getRunInfo());
	for (auto name : ArbGenEnum::allAgs) {
		arbGens[(int)name].checkSave(getProfile().configLocation, mainWin->getRunInfo());
	}
	//intensityAgilent.checkSave(getProfile().configLocation, mainWin->getRunInfo());
}

std::string QtScriptWindow::getSystemStatusString (){
	//std::string status = "Intensity Agilent:\n\t" + intensityAgilent.getDeviceIdentity();
	std::string status;
	for (auto name : ArbGenEnum::allAgs) {
		status += arbGens[(int)name].initSettings.deviceName + ":\n\t" + arbGens[(int)name].getDeviceIdentity();
		status += "\t";
		status += "Attached trigger line is \n\t\t";
		{
			status += "(" + str(arbGens[(int)name].initSettings.triggerRow) + "," + str(arbGens[(int)name].initSettings.triggerNumber) + ") ";
		}
		status += "\n";
	}
	status += "GIGAMOOG:\n\t";
	if (!GIGAMOOG_SAFEMODE) {
		status += str("GIGAMOOG System is Active at " + GIGAMOOG_IPADDRESS + " and port," + str(GIGAMOOG_IPPORT) + "\n\t");
		status += "Attached trigger line is \n\t\t";
		for (const auto& gmtrig : GM_TRIGGER_LINE) {
			status += "(" + str(gmtrig.first) + "," + str(gmtrig.second) + ") ";
		}
		status += "\n";
	}
	else {
		status += "\tGIGAMOOG System is disabled! Enable in \"constants.h\" \n";
	}
	return status;
}

/* 
  This function retuns the names (just the names) of currently active scripts.
*/
scriptInfo<std::string> QtScriptWindow::getScriptNames (){
	scriptInfo<std::string> names;
	names.master = masterScript.getScriptName ();
	names.gmoog = gigaMoog.gmoogScript.getScriptName();
	//names.intensityAgilent = intensityAgilent.arbGenScript.getScriptName();
	return names;
}

/*
  This function returns indicators of whether a given script has been saved or not.
*/
scriptInfo<bool> QtScriptWindow::getScriptSavedStatuses (){
	scriptInfo<bool> status;
	//status.intensityAgilent = intensityAgilent.arbGenScript.savedStatus();
	status.master = masterScript.savedStatus ();
	status.gmoog = gigaMoog.gmoogScript.savedStatus();
	return status;
}

/*
  This function returns the current addresses of all files in all scripts.
*/
scriptInfo<std::string> QtScriptWindow::getScriptAddresses (){
	scriptInfo<std::string> addresses;
	//addresses.intensityAgilent = intensityAgilent.arbGenScript.getScriptPathAndName();
	addresses.master = masterScript.getScriptPathAndName ();
	addresses.gmoog = gigaMoog.gmoogScript.getScriptPathAndName();
	return addresses;
}

void QtScriptWindow::setIntensityDefault() 
{
	try {
		for (auto name : ArbGenEnum::allAgs) {
			arbGens[(int)name].setDefault(1);
			arbGens[(int)name].setDefault(2);
		}
	}
	catch (ChimeraError& err) {
		reportErr(err.qtrace());
	}
}

/// Commonly Called Functions
/*
	The following set of functions, mostly revolving around saving etc. of the script files, are called by all of the
	window objects because they are associated with the menu at the top of each screen
*/

void QtScriptWindow::updateArbGen(ArbGenEnum::name name) {
	try {
		updateConfigurationSavedStatus(false);
		arbGens[(int)name].checkSave(getProfile().configLocation, mainWin->getRunInfo());
		arbGens[(int)name].readGuiSettings();
	}
	catch (ChimeraError&) {
		throwNested("Failed to update arbGens.");
	}
}


void QtScriptWindow::newArbGenScript(ArbGenEnum::name name) 
{
	try {
		arbGens[(int)name].verifyScriptable();
		mainWin->updateConfigurationSavedStatus(false);
		arbGens[(int)name].checkSave(mainWin->getProfileSettings().configLocation, mainWin->getRunInfo());
		arbGens[(int)name].arbGenScript.newScript();
		arbGens[(int)name].arbGenScript.updateScriptNameText(mainWin->getProfileSettings().configLocation);
	}
	catch (ChimeraError& err) {
		reportErr(err.qtrace());
	}
}

void QtScriptWindow::openArbGenScript(ArbGenEnum::name name, IChimeraQtWindow* parent)
{
	try {
		arbGens[(int)name].verifyScriptable();
		updateConfigurationSavedStatus(false);
		arbGens[(int)name].checkSave(getProfile().configLocation, mainWin->getRunInfo());
		std::string openFileName = openWithExplorer(parent, Script::ARBGEN_SCRIPT_EXTENSION, CONFIGURATION_PATH);
		arbGens[(int)name].arbGenScript.openParentScript(openFileName, getProfile().configLocation, 
			mainWin->getRunInfo());
		arbGens[(int)name].arbGenScript.updateScriptNameText(getProfile().configLocation);
	}
	catch (ChimeraError& err) {
		reportErr(err.qtrace());
	}
}

void QtScriptWindow::saveArbGenScript(ArbGenEnum::name name) {
	try {
		arbGens[(int)name].verifyScriptable();
		arbGens[(int)name].arbGenScript.saveScript(getProfile().configLocation, mainWin->getRunInfo());
		arbGens[(int)name].arbGenScript.updateScriptNameText(getProfile().configLocation);
	}
	catch (ChimeraError& err) {
		reportErr(err.qtrace());
	}
}

void QtScriptWindow::saveArbGenScriptAs(ArbGenEnum::name name, IChimeraQtWindow* parent) {
	try {
		arbGens[(int)name].verifyScriptable();
		updateConfigurationSavedStatus(false);
		std::string extensionNoPeriod = arbGens[(int)name].arbGenScript.getExtension();
		if (extensionNoPeriod.size() == 0) {
			return;
		}
		extensionNoPeriod = extensionNoPeriod.substr(1, extensionNoPeriod.size());
		std::string newScriptAddress = saveWithExplorer(parent, extensionNoPeriod, getProfileSettings());
		arbGens[(int)name].arbGenScript.saveScriptAs(newScriptAddress, mainWin->getRunInfo());
		arbGens[(int)name].arbGenScript.updateScriptNameText(getProfile().configLocation);
	}
	catch (ChimeraError& err) {
		reportErr(err.qtrace());
	}
}




// just a quick shortcut.
profileSettings QtScriptWindow::getProfile (){
	return mainWin->getProfileSettings ();
}

void QtScriptWindow::windowOpenConfig (ConfigStream& configFile){
	try{
		ConfigSystem::initializeAtDelim (configFile, "SCRIPTS");
	}
	catch (ChimeraError&){
		reportErr ("Failed to initialize configuration file at scripting window entry point \"SCRIPTS\".");
		return;
	}
	try{
		auto getlineFunc = ConfigSystem::getGetlineFunc (configFile.ver);
		std::string masterName/*, gmoogName*/;
		// order should match the windowsaveconfig
		getlineFunc (configFile, masterName);
		//getlineFunc(configFile, gmoogName);
		ConfigSystem::checkDelimiterLine (configFile, "END_SCRIPTS");
		try {
			openMasterScript(masterName);
		}
		catch (ChimeraError& err) {
			auto answer = QMessageBox::question(this, "Open Failed", "ERROR: Failed to open master script file: "
				+ qstr(masterName) + ", with error \r\n" + err.qtrace() + "\r\nAttempt to find file yourself?");
			if (answer == QMessageBox::Yes) {
				openMasterScript(openWithExplorer(nullptr, "mScript", CONFIGURATION_PATH));
			}
		}

		ConfigSystem::standardOpenConfig(configFile, gigaMoog.getDelim(), &gigaMoog);
		try {
			openGMoogScript(gigaMoog.scriptAddress);
		}
		catch (ChimeraError& err) {
			auto answer = QMessageBox::question(this, "Open Failed", "ERROR: Failed to open master script file: "
				+ qstr(gigaMoog.scriptAddress) + ", with error \r\n" + err.qtrace() + "\r\nAttempt to find file yourself?");
			if (answer == QMessageBox::Yes) {
				openGMoogScript(openWithExplorer(nullptr, "gScript", CONFIGURATION_PATH));
			}
		}
		for (auto name : ArbGenEnum::allAgs) {
			deviceOutputInfo info;
			ConfigSystem::stdGetFromConfig(configFile, arbGens[(int)name].getCore(), info, Version("1.0"));
			arbGens[(int)name].setOutputSettings(info);
			arbGens[(int)name].updateSettingsDisplay(getProfileSettings().configLocation, mainWin->getRunInfo());
		}


		considerScriptLocations();
	}
	catch (ChimeraError& err)	{
		reportErr ("Scripting Window failed to read parameters from the configuration file.\n\n" + err.qtrace ());
	}
}

void QtScriptWindow::newMasterScript (){
	try {
		masterScript.checkSave (getProfile ().configLocation, mainWin->getRunInfo());
		masterScript.newScript ();
		updateConfigurationSavedStatus (false);
		masterScript.updateScriptNameText (getProfile ().configLocation);
	}
	catch (ChimeraError & err) {
		reportErr (err.qtrace ());
	}
}

void QtScriptWindow::openMasterScript (IChimeraQtWindow* parent){
	try	{
		masterScript.checkSave (getProfile ().configLocation, mainWin->getRunInfo());
		std::string openName = openWithExplorer (parent, Script::MASTER_SCRIPT_EXTENSION, CONFIGURATION_PATH);
		masterScript.openParentScript (openName, getProfile ().configLocation, mainWin->getRunInfo());
		updateConfigurationSavedStatus (false);
		masterScript.updateScriptNameText (getProfile ().configLocation);
	}
	catch (ChimeraError& err){
		reportErr ("Open Master Script Failed: " + err.qtrace () + "\r\n");
	}
}

void QtScriptWindow::openMasterScript(std::string name, bool askMove) {
	masterScript.openParentScript(name, getProfile().configLocation, mainWin->getRunInfo(), askMove);
}

void QtScriptWindow::saveMasterScript (){
	if (masterScript.isFunction ())	{
		masterScript.saveAsFunction ();
		return;
	}
	masterScript.saveScript (getProfile ().configLocation, mainWin->getRunInfo());
	masterScript.updateScriptNameText (getProfile ().configLocation);
}

void QtScriptWindow::saveMasterScriptAs (IChimeraQtWindow* parent){
	std::string extensionNoPeriod = masterScript.getExtension ();
	if (extensionNoPeriod.size () == 0)	{
		return;
	}
	extensionNoPeriod = extensionNoPeriod.substr (1, extensionNoPeriod.size ());
	std::string newScriptAddress = saveWithExplorer (parent, extensionNoPeriod, getProfileSettings ());
	masterScript.saveScriptAs (newScriptAddress, mainWin->getRunInfo());
	updateConfigurationSavedStatus (false);
	masterScript.updateScriptNameText (getProfile ().configLocation);
}

void QtScriptWindow::newMasterFunction (){
	try{
		masterScript.newFunction ();
	}
	catch (ChimeraError& exception){
		reportErr ("New Master function Failed: " + exception.qtrace () + "\r\n");
	}
}

void QtScriptWindow::reloadMasterFunction()
{
	try {
		masterScript.loadFunctions();
	}
	catch (ChimeraError& exception) {
		reportErr("New Master function Failed: " + exception.qtrace() + "\r\n");
	}
}

void QtScriptWindow::saveMasterFunction (){
	try{
		masterScript.saveAsFunction ();
	}
	catch (ChimeraError& exception){
		reportErr ("Save Master Script Function Failed: " + exception.qtrace () + "\r\n");
	}
}

void QtScriptWindow::deleteMasterFunction (){
	// todo. Right now you can just delete the file itself...
}

void QtScriptWindow::newGMoogScript()
{
	try {
		gigaMoog.gmoogScript.checkSave(getProfile().configLocation, mainWin->getRunInfo());
		gigaMoog.gmoogScript.newScript();
		updateConfigurationSavedStatus(false);
		gigaMoog.gmoogScript.updateScriptNameText(getProfile().configLocation);
	}
	catch (ChimeraError& err) {
		reportErr(err.qtrace());
	}
}

void QtScriptWindow::openGMoogScript(IChimeraQtWindow* parent)
{
	try {
		gigaMoog.gmoogScript.checkSave(getProfile().configLocation, mainWin->getRunInfo());
		std::string openName = openWithExplorer(parent, Script::GMOOG_SCRIPT_EXTENSION, CONFIGURATION_PATH);
		gigaMoog.gmoogScript.openParentScript(openName, getProfile().configLocation, mainWin->getRunInfo());
		updateConfigurationSavedStatus(false);
		gigaMoog.gmoogScript.updateScriptNameText(getProfile().configLocation);
	}
	catch (ChimeraError& err) {
		reportErr("Open GigaMoog Script Failed: " + err.qtrace() + "\r\n");
	}
}

void QtScriptWindow::openGMoogScript(std::string name)
{
	gigaMoog.gmoogScript.openParentScript(name, getProfile().configLocation, mainWin->getRunInfo());
}

void QtScriptWindow::saveGMoogScript()
{
	gigaMoog.gmoogScript.saveScript(getProfile().configLocation, mainWin->getRunInfo());
	gigaMoog.gmoogScript.updateScriptNameText(getProfile().configLocation);
}

void QtScriptWindow::saveGMoogScriptAs(IChimeraQtWindow* parent)
{
	std::string extensionNoPeriod = gigaMoog.gmoogScript.getExtension();
	if (extensionNoPeriod.size() == 0) {
		return;
	}
	extensionNoPeriod = extensionNoPeriod.substr(1, extensionNoPeriod.size());
	std::string newScriptAddress = saveWithExplorer(parent, extensionNoPeriod, getProfileSettings());
	gigaMoog.gmoogScript.saveScriptAs(newScriptAddress, mainWin->getRunInfo());
	updateConfigurationSavedStatus(false);
	gigaMoog.gmoogScript.updateScriptNameText(getProfile().configLocation);
}

void QtScriptWindow::saveAllScript()
{
	saveMasterScript();
	saveGMoogScript();
	for (auto name : ArbGenEnum::allAgs) {
		saveArbGenScript(name);
	}
}

void QtScriptWindow::windowSaveConfig (ConfigStream& saveFile){
	scriptInfo<std::string> addresses = getScriptAddresses ();
	// order matters!
	saveFile << "SCRIPTS\n";
	saveFile << "/*Master Script Address:*/ " << addresses.master << "\n";
	//saveFile << "/*GigaMoog Script Address:*/ " << addresses.gmoog << "\n";
	saveFile << "END_SCRIPTS\n";
	gigaMoog.handleSaveConfig(saveFile);
	for (auto name : ArbGenEnum::allAgs) {
		arbGens[(int)name].handleSavingConfig(saveFile, getProfileSettings().configLocation, mainWin->getRunInfo());
	}
}

void QtScriptWindow::checkMasterSave (){
	masterScript.checkSave (getProfile ().configLocation, mainWin->getRunInfo());
	gigaMoog.gmoogScript.checkSave(getProfile().configLocation, mainWin->getRunInfo());
}

void QtScriptWindow::considerScriptLocations() {
	for (auto name : ArbGenEnum::allAgs) {
		arbGens[(int)name].arbGenScript.considerCurrentLocation(getProfile().configLocation, mainWin->getRunInfo());
	}
	masterScript.considerCurrentLocation(getProfile().configLocation, mainWin->getRunInfo());
	gigaMoog.gmoogScript.considerCurrentLocation(getProfile().configLocation, mainWin->getRunInfo());
}

//void QtScriptWindow::updateProfile (std::string text){
//	//profileDisplay.update (text);
//}

profileSettings QtScriptWindow::getProfileSettings (){
	return mainWin->getProfileSettings ();
}

void QtScriptWindow::updateConfigurationSavedStatus (bool status){
	mainWin->updateConfigurationSavedStatus (status);
}

void QtScriptWindow::fillExpDeviceList (DeviceList& list) {
	for (auto name : ArbGenEnum::allAgs) {
		list.list.push_back(arbGens[(int)name].getCore());
	}
	list.list.push_back(gigaMoog.getCore());
}

std::vector<std::reference_wrapper<ArbGenSystem>> QtScriptWindow::getArbGenSystem()
{
	std::vector<std::reference_wrapper<ArbGenSystem>> ags;
	for (ArbGenSystem& ag : arbGens) {
		ags.push_back(ag);
	}
	return ags;
}

std::vector<std::reference_wrapper<ArbGenCore>> QtScriptWindow::getArbGenCore()
{
	std::vector<std::reference_wrapper<ArbGenCore>> agCores;
	for (ArbGenSystem& ag : arbGens) {
		agCores.push_back(ag.getCore());
	}
	return agCores;
}