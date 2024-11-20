#include "stdafx.h"
#include "CommandModulator.h"
#include "PrimaryWindows/IChimeraQtWindow.h"
#include "PrimaryWindows/QtMainWindow.h"
#include "PrimaryWindows/QtAndorWindow.h"
#include "PrimaryWindows/QtAuxiliaryWindow.h"
#include "PrimaryWindows/QtAnalysisWindow.h"
#include "PrimaryWindows/QtMakoWindow.h"
#include "PrimaryWindows/QtScriptWindow.h"
#include <GeneralUtilityFunctions/commonFunctions.h>
#include <algorithm>

CommandModulator::CommandModulator(IChimeraSystem* parentSys):
	QObject(parentSys)
{}

void CommandModulator::initialize(IChimeraQtWindow* win)
{
	mainWin = win->mainWin;
	andorWin = win->andorWin;
	scriptWin = win->scriptWin;
	auxWin = win->auxWin;
	analysisWin = win->analysisWin;
	makoWin1 = win->makoWin1;
	makoWin2 = win->makoWin2;
}

void CommandModulator::openConfiguration(QString addressName, ErrorStatus& status)
{
	auto filePath = convertToUnixPath(CONFIGURATION_PATH + str(addressName));
	mainWin->profile.openConfigFromPath(filePath, mainWin);
}

void CommandModulator::openMasterScript(QString addressName, ErrorStatus& status)
{
	try {
		status.error = false;
		auto filePath = convertToUnixPath(CONFIGURATION_PATH + str(addressName));
		scriptWin->openMasterScript(filePath, false);
	}
	catch (ChimeraError& err) {
		mainWin->reportErr(err.qtrace());
		status.error = true;
		status.errorMsg = err.trace();
	}
}

void CommandModulator::saveAll(ErrorStatus& status)
{
	try {
		status.error = false;
		scriptWin->saveAllScript();
		mainWin->profile.saveConfiguration(mainWin, false);
		mainWin->masterConfig.save(mainWin, auxWin, andorWin);
	}
	catch (ChimeraError& err) {
		mainWin->reportErr(err.qtrace());
		status.error = true;
		status.errorMsg = err.trace();
	}
}

void CommandModulator::startExperiment(QString expDataName, ErrorStatus& status)
{
	try {
		status.error = false;
		CalibrationManager& calManager = auxWin->getCalibManager();
		if (calManager.isCalibrationRunning()) {
			thrower("Calibration is still running.");
		}

		AllExperimentInput input;
		if (mainWin->masterIsRunning()) {
			thrower("Experiment is still running.");
		}
		andorWin->setTimerText("Starting...");
		saveAll(status);
		commonFunctions::prepareMasterThread(0, mainWin, input, true, true, true, true);
		input.masterInput->expType = ExperimentType::Normal;

		commonFunctions::logStandard(input, andorWin->getLogger(), str(expDataName));
		commonFunctions::startExperimentThread(mainWin, input);
	
	}
	catch (ChimeraError& err) {
		mainWin->reportErr("EXITED WITH ERROR!\n " + err.qtrace());
		mainWin->reportStatus("EXITED WITH ERROR!\r\nInitialized Default Waveform\r\n");
		andorWin->setTimerText("ERROR!");
		andorWin->assertOff();
		status.error = true;
		status.errorMsg = err.trace();
	}
}

void CommandModulator::abortExperiment(bool keepData, QString dataName, ErrorStatus& status)
{
	status.error = false;
	bool andorAborted = false, masterAborted = false;
	andorWin->wakeRearranger();
	try {
		if (mainWin->expIsRunning()) {
			commonFunctions::abortMaster(mainWin);
			masterAborted = true;
		}
		andorWin->assertOff();
		andorWin->assertDataFileClosed();
	}
	catch (ChimeraError& err) {
		mainWin->reportErr("Abort Master thread exited with Error! Error Message: "
			+ err.qtrace());
		mainWin->reportStatus("Abort Master thread exited with Error!\r\n");
		andorWin->setTimerText("ERROR!");
		status.error = true;
		status.errorMsg += "\n" + err.trace();
	}

	try {
		//below is adapted commonFunctions::abortCamera(mainWin);
		if (!andorWin->cameraIsRunning()) {
			mainWin->reportErr("System was not running. Can't Abort.\r\n");
		}
		else {
			// abort acquisition if in progress
			Sleep(200); // leave time for other stuff to save into datafile
			andorWin->abortCameraRun(false);
			mainWin->reportStatus("Aborted Camera Operation.\r\n");
			andorAborted = true;

			if (!keepData) {
				try {
					andorWin->getLogger().deleteFile(str(dataName));
				}
				catch (ChimeraError& err) {
					andorWin->reportErr(qstr(err.trace()));
					status.error = true;
					status.errorMsg += "\n" + err.trace();
				}
			}
		}
	}
	catch (ChimeraError& err) {
		mainWin->reportErr("Andor Camera threw error while aborting! Error: " + err.qtrace());
		mainWin->reportStatus("Abort camera threw error\r\n");
		andorWin->setTimerText("ERROR!");
		status.error = true;
		status.errorMsg += "\n" + err.trace();
	}
	
	if (!andorAborted && !masterAborted) {
		for (auto& dev : mainWin->getDevices().list) {
			mainWin->handleColorboxUpdate("Black", qstr(dev.get().getDelim()));
		}
		mainWin->reportErr("Andor camera, Master, and Basler camera were not running. "
			"Can't Abort.\r\n");
	}
}

void CommandModulator::isExperimentRunning(bool& running, ErrorStatus& status)
{
	status.error = false;
	running = mainWin->masterIsRunning();
}

void CommandModulator::startCalibration(QString calName, ErrorStatus& status)
{
	try {
		status.error = false;
		CalibrationManager& calManager = auxWin->getCalibManager();
		if (mainWin->masterIsRunning()) {
			thrower("Experiment is still running.");
		}
		if (calManager.isCalibrationRunning()) {
			thrower("Calibration is still running.");
		}

		std::string calNameStr = str(calName);
		auto& cals = calManager.calibrations; // need reference as the calibrateThreaded input
		auto it = std::find_if(cals.begin(), cals.end(), [calNameStr](const calSettings& calS) {
			return (calS.result.calibrationName == calNameStr);
			});
		if (it == cals.end()) {
			thrower("Failed to find the calibration name in the current calibration list. Callibration name is: " + calNameStr);
		}
		auto& tobeCalibrated = *it;
		calManager.calibrateThreaded(tobeCalibrated, -1); // unsigned which in calibrateThreaded is not used
	}
	catch (ChimeraError& err) {
		mainWin->reportErr("ERROR in remote calibration!\n " + err.qtrace());
		status.error = true;
		status.errorMsg = err.trace();
	}
}

void CommandModulator::isCalibrationRunning(bool& running, ErrorStatus& status)
{
	status.error = false;
	CalibrationManager& calManager = auxWin->getCalibManager();
	running = calManager.isCalibrationRunning();
}

void CommandModulator::setStaticDDS(QString ddsfreqStr, QString channelStr, ErrorStatus& status)
{
	unsigned channel;
	std::string ddsfreq = str(ddsfreqStr);
	try {
		channel = boost::lexical_cast<unsigned>(str(channelStr));
	}
	catch (boost::bad_lexical_cast&) {
		status.error = true;
		status.errorMsg = "Error\nError in converting command argument to number";
		return;
	}

	auxWin->reportStatus("----------------------\r\nSetting static Ddss... ");
	try {
		auto& staticDds = analysisWin->getStaticDds();
		staticDds.setDdsEditValue(ddsfreq, channel);
		staticDds.handleProgramNowPress(auxWin->getUsableConstants());
	}
	catch (ChimeraError& err) {
		mainWin->reportStatus(": " + err.qtrace() + "\r\n");
		mainWin->reportErr(err.qtrace());
		status.error = true;
		status.errorMsg = err.trace();
	}
}

void CommandModulator::setTTL(QString name, QString value, ErrorStatus& status)
{
	unsigned int row, number;
	auto ttlIdn = auxWin->getTtlCore().getNameIdentifier(str(name), row, number);
	if (ttlIdn == -1) {
		status.error = true;
		status.errorMsg = "Error\nError in converting command argument " + str(name) + " to TtlIdentifier ";
		return;
	}
	bool ttlVal;
	try {
		ttlVal = boost::lexical_cast<bool>(str(value));
	}
	catch (boost::bad_lexical_cast&) {
		status.error = true;
		status.errorMsg = "Error\nError in converting command argument " + str(value) + " to bool";
		return;
	}
	try {
		auxWin->getTtlSystem().setSingleTtlGui(row, number, ttlVal);
	}
	catch (ChimeraError& err) {
		status.error = true;
		status.errorMsg = err.trace();
	}
}

void CommandModulator::setDAC(QString name, QString value, ErrorStatus& status)
{
	if (name == "") {
		auxWin->reportStatus("----------------------\r\nSetting Dacs... ");
		try {
			status.error = false;
			auxWin->reportStatus("Setting Dacs...\r\n");
			auxWin->getAoSys().handleSetDacsButtonPress(true);
			auxWin->getTtlSystem().setTtlStatus(auxWin->getTtlSystem().getCurrentStatus());
			auxWin->reportStatus("Finished Setting Dacs.\r\n");
		}
		catch (ChimeraError& err) {
			mainWin->reportStatus(": " + err.qtrace() + "\r\n");
			mainWin->reportErr(err.qtrace());
			status.error = true;
			status.errorMsg = err.trace();
		}
	}
	else {
		int dacIdn = auxWin->getAoSys().getCore().getDacIdentifier(str(name));
		if (dacIdn == -1) {
			status.error = true;
			status.errorMsg = "Error\nError in converting command argument " + str(name) + " to DacIdentifier ";
			return;
		}
		double dacVal;
		try {
			dacVal = boost::lexical_cast<double>(str(value));
		}
		catch (boost::bad_lexical_cast&) {
			status.error = true;
			status.errorMsg = "Error\nError in converting command argument " + str(value) + " to number";
			return;
		}
		try {
			auxWin->getAoSys().setSingleDacGui(dacIdn, dacVal);
			auxWin->getTtlSystem().getCore().FPGAForceOutput(auxWin->getTtlSystem().getCurrentStatus());
		}
		catch (ChimeraError& err) {
			status.error = true;
			status.errorMsg = err.trace();
		}
	}


}

void CommandModulator::setOL(ErrorStatus& status)
{
	auxWin->reportStatus("----------------------\r\nSetting Offsetlocks... ");
	try {
		status.error = false;
		auxWin->reportStatus("Setting Ols...\r\n");
		auxWin->getOlSys().handleSetOlsButtonPress(auxWin->getTtlSystem().getCore(), auxWin->getTtlSystem().getCurrentStatus());
		auxWin->reportStatus("Finished Setting Offsetlocks.\r\n");
	}
	catch (ChimeraError& err) {
		mainWin->reportStatus(": " + err.qtrace() + "\r\n");
		mainWin->reportErr(err.qtrace());
		status.error = true;
		status.errorMsg = err.trace();
	}
}

void CommandModulator::setDDS(ErrorStatus& status)
{
	auxWin->reportStatus("----------------------\r\nSetting DDSs... ");
	try {
		status.error = false;
		auxWin->reportStatus("Setting Ddss...\r\n");
		auxWin->getDdsSys().handleSetDdsButtonPress(true);
		auxWin->getTtlSystem().setTtlStatus(auxWin->getTtlSystem().getCurrentStatus());
		auxWin->reportStatus("Finished Setting DDSs.\r\n");
	}
	catch (ChimeraError& err) {
		mainWin->reportStatus(": " + err.qtrace() + "\r\n");
		mainWin->reportErr(err.qtrace());
		status.error = true;
		status.errorMsg = err.trace();
	}
}

void CommandModulator::startMako(QString whichMako, ErrorStatus& status)
{
	auto* cam = getMakoCameraPtr(whichMako, status);
	if (status.error) {
		return;
	}
	try {
		cam->acquisitionStartStopFromCtrler("AcquisitionStart");
	}
	catch (ChimeraError& e) {
		status.error = true;
		status.errorMsg = "Error in starting Mako: " + e.trace();
	}
}

void CommandModulator::stopMako(QString whichMako, ErrorStatus& status)
{
	auto* cam = getMakoCameraPtr(whichMako, status);
	if (status.error) {
		return;
	}
	try {
		cam->acquisitionStartStopFromCtrler("AcquisitionStop");
	}
	catch (ChimeraError& e) {
		status.error = true;
		status.errorMsg = "Error in stopping Mako: " + e.trace();
	}
}

void CommandModulator::getMakoImage(QString whichMako, QVector<char>& imgResult, ErrorStatus& status)
{
	auto* cam = getMakoCameraPtr(whichMako, status);
	if (status.error) {
		return;
	}

	try {
		auto img = cam->getCurrentImageInBuffer().toStdVector();
		imgResult = QVector<char>::fromStdVector(vectorToVectorChar<double>(img));
	}
	catch (ChimeraError& e) {
		status.error = true;
		status.errorMsg = "Error in getting Mako image: " + e.trace();
	}
}

void CommandModulator::getMakoImageDimension(QString whichMako, QVector<char>& imgDimParamResult, ErrorStatus& status)
{
	auto* cam = getMakoCameraPtr(whichMako, status);
	if (status.error) {
		return;
	}

	try {
		auto& camCore = cam->getMakoCore();
		camCore.updateCurrentSettings();
		auto dim = camCore.getRunningSettings().dims;
		auto imgDimParam = std::vector<double>({ static_cast<double>(dim.top), static_cast<double>(dim.left), 
			static_cast<double>(dim.bottom), static_cast<double>(dim.right) });
		imgDimParamResult = QVector<char>::fromStdVector(vectorToVectorChar<double>(imgDimParam));
	}
	catch (ChimeraError& e) {
		status.error = true;
		status.errorMsg = "Error in getting Mako image: " + e.trace();
	}
}

void CommandModulator::getMakoFeatureValue(QString whichMako, QString featureName, QString featureType, QVector<char>& featureValue, ErrorStatus& status)
{
	auto* cam = getMakoCameraPtr(whichMako, status);
	if (status.error) {
		return;
	}
	auto& makoCtrl = cam->getMakoCore().getMakoCtrl();
	try {
		std::string errStr;
		//std::any value;
		std::vector<char> result;
		if (featureType == "string") {
			auto value = makoCtrl.getFeatureValue<std::string>(str(featureName), errStr);
			result = std::vector<char>(value.begin(), value.end());
			result = vectorToVectorChar(result);
		}
		else if (featureType == "double") {
			double value = makoCtrl.getFeatureValue<double>(str(featureName), errStr);
			result = vectorToVectorChar<double>(std::vector<double>({ value }));
		}
		else if (featureType == "int") {
			int value = makoCtrl.getFeatureValue<VmbInt64_t>(str(featureName), errStr);
			result = vectorToVectorChar<int>(std::vector<int>({ value }));
		}
		else {
			status.error = false;
			status.errorMsg = "Error in getting Mako feature " + str(featureName) + " : Unrecongized feature type: " + str(featureType);
		}
		featureValue = QVector<char>::fromStdVector(result);
	}
	catch (ChimeraError& e) {
		status.error = true;
		status.errorMsg = "Error in getting Mako feature " + str(featureName) + " : " + e.trace();
	}
}

void CommandModulator::setMakoFeatureValue(QString whichMako, QString featureName, QString featureType, QString featureValue, ErrorStatus& status)
{
	auto* cam = getMakoCameraPtr(whichMako, status);
	if (status.error) {
		return;
	}
	auto& makoCtrl = cam->getMakoCore().getMakoCtrl();
	try {
		std::string errStr;
		//std::any value;
		std::vector<char> result;
		if (featureType == "string") {
			makoCtrl.setFeatureValue(str(featureName), featureValue.toStdString().c_str(), errStr); // implicitly using const char*
		}
		else if (featureType == "double") {
			double value;
			try {
				value = boost::lexical_cast<double>(str(featureValue));
			}
			catch (boost::bad_lexical_cast& e){
				throwNested("Error in converting " + str(featureName) + "'s value.");
			}
			makoCtrl.setFeatureValue<double>(str(featureName), value, errStr);
		}
		else if (featureType == "int") {
			int value;
			try {
				value = boost::lexical_cast<int>(str(featureValue));
			}
			catch (boost::bad_lexical_cast& e) {
				throwNested("Error in converting " + str(featureName) + "'s value.");
			}
			makoCtrl.setFeatureValue<VmbInt64_t>(str(featureName), value, errStr);
		}
		else {
			status.error = false;
			status.errorMsg = "Error in setting Mako feature " + str(featureName) + " : Unrecongized feature type: " + str(featureType);
		}
	}
	catch (ChimeraError& e) {
		status.error = true;
		status.errorMsg = "Error in getting Mako feature " + str(featureName) + " : " + e.trace();
	}
}

void CommandModulator::setPicoScrewPosition(QString whichScrew, QString value, QString bUpdate, ErrorStatus& status)
{
	auto& picoSys = auxWin->getPsSys();
	unsigned channel;
	try {
		channel = boost::lexical_cast<unsigned>(str(whichScrew));
	}
	catch (boost::bad_lexical_cast&) {
		status.error = true;
		status.errorMsg = "Error\nError in converting command argument to number";
		return;
	}
	int pos;
	try {
		pos = boost::lexical_cast<int>(str(value));
	}
	catch (boost::bad_lexical_cast&) {
		status.error = true;
		status.errorMsg = "Error\nError in converting command argument to number";
		return;
	}
	bool update;
	try {
		update = boost::lexical_cast<bool>(str(bUpdate));
	}
	catch (boost::bad_lexical_cast&) {
		status.error = true;
		status.errorMsg = "Error\nError in converting command argument " + str(value) + " to bool";
		return;
	}
	picoSys.updateCurrentEditValue(channel - 1, pos);
	if (update) {
		picoSys.handleProgramNowPress(auxWin->getUsableConstants());
	}
}

void CommandModulator::getPicoScrewPositions(QVector<char>& positionValue, ErrorStatus& status)
{
	try {
		auto& picoCore = auxWin->getPsSys().getCore();
		std::vector<int> position;
		for (auto id : range(PICOSCREW_NUM)) {
			position.push_back(picoCore.motorPosition(id));
		}
		std::vector<char> positionChar = vectorToVectorChar(position);
		positionValue = QVector<char>::fromStdVector(positionChar);
	}
	catch (ChimeraError& e) {
		status.error = true;
		status.errorMsg = "Error\nError in getPicoScrewPositions " + e.trace();
		return;
	}
}

void CommandModulator::setPicoScrewHomes(ErrorStatus& status)
{
	try {
		for (auto ch : range(PICOSCREW_NUM)) {
			auxWin->getPsSys().getCore().setHomePosition(ch);
		}
	}
	catch (ChimeraError& e) {
		status.error = true;
		status.errorMsg = "Error\nError in setPicoScrewHomes " + e.trace();
		return;
	}
}


template<typename T>
static std::vector<char> CommandModulator::vectorToVectorChar(const std::vector<T>& data)
{
	std::vector<char> buffer; // size of vector + vector

	std::size_t size = data.size();
	buffer.reserve(sizeof(size) + sizeof(T) * size);
	// Copy the size of the vector
	buffer.insert(buffer.end(), reinterpret_cast<const char*>(&size), reinterpret_cast<const char*>(&size) + sizeof(size)); // Little-endian for intel x86 and AMD
	// Copy the vector data
	//buffer.insert(buffer.end(), reinterpret_cast<const char*>(data.data()), reinterpret_cast<const char*>(data.data()) + sizeof(T) * size);
	buffer.resize(buffer.size() + sizeof(T) * size); // Resize the buffer to accommodate new data
	std::memcpy(buffer.data() + sizeof(size), data.data(), sizeof(T) * size); // Copy the data
	return buffer;
}

std::string CommandModulator::convertToUnixPath(std::string mixedPath)
{
	// not that the mixed syntax path wouldn't work. 
	// Just to make Chimera consistent with one type of format so that 
	// the loc of scripts etc can be determined to see if they are at the same loc as config
	
	// Replace all occurrences of '\\' with '/'
	std::replace(mixedPath.begin(), mixedPath.end(), '\\', '/');
	std::string result;
	bool lastWasSlash = false;

	for (char ch : mixedPath) {
		if (ch == '/') {
			if (!lastWasSlash) {
				result += ch;
				lastWasSlash = true;
			}
		}
		else {
			result += ch;
			lastWasSlash = false;
		}
	}
	return result;
}

MakoCamera* CommandModulator::getMakoCameraPtr(QString whichMako, ErrorStatus& status)
{
	// get the corresponding camera
	QStringList makos = { "mako1", "mako2", "mako3", "mako4" };
	MakoCamera* cam;
	if (makos.contains(whichMako)) {
		QString numberStr = whichMako.mid(4); // Get the substring after "mako"
		int number = numberStr.toInt();
		if (whichMako == "mako1" || whichMako == "mako2") {
			cam = makoWin1->getMakoCam((number - 1) % 2);
		}
		else if (whichMako == "mako3" || whichMako == "mako4") {
			cam = makoWin2->getMakoCam((number - 1) % 2);
		}
	}
	else {
		status.error = true;
		status.errorMsg = "whichMako = " + str(whichMako) + " does not match the mako list. Valid lists are " + str(makos.join(", "));
		cam = nullptr;
	}
	return cam;
}

