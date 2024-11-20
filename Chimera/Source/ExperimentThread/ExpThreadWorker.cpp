#include "stdafx.h"
#include "ExpThreadWorker.h"
#include <ConfigurationSystems/ConfigSystem.h>
#include <MiscellaneousExperimentOptions/Repetitions.h>
#include <Scripts/Script.h>
#include <DataLogging/DataLogger.h>
#include <boost/algorithm/string/replace.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string.hpp>


ExpThreadWorker::ExpThreadWorker (ExperimentThreadInput* input_, std::atomic<bool>& expRunning) 
	: experimentIsRunning(expRunning){
	experimentIsRunning = true; // for tcpclient to know exp is about to run
	input = std::unique_ptr< ExperimentThreadInput >((ExperimentThreadInput*)input_);
}

ExpThreadWorker::~ExpThreadWorker () {
}

/*this is called from commonFunctions::startExperimentThread, which further call mainWin->startExperimentThread, 
where it create a new QThread object and a new ExpThreadWorker. Then move ExpThreadWorker to the new QThread with 
QObject::moveToThread. The thread is started with QThread::start and the started signal from QThread is connected to
ExpThreadWorker::process.
The input content can be changed at the ExperimentThreadInput struct and remember to modefy the constructor to fill
the correct initial value.*/
void ExpThreadWorker::process ()
{
	experimentThreadProcedure();
	emit mainProcessFinish ();
}

/*
 * The workhorse of actually running experiments. This thread procedure analyzes all of the GUI settings and current
 * configuration settings to determine how to program and run the experiment.
 */
void ExpThreadWorker::experimentThreadProcedure () {
	auto startTime = chronoClock::now ();
	experimentIsRunning = true;
	emit notification (qstr ("Starting Experiment " + input->profile.configuration + "...\n"));
	ExpRuntimeData expRuntime;
	isPaused = false;
	try {
	 	for (auto& device : input->devices.list) {
	 		emit updateBoxColor ("Violet", device.get ().getDelim ().c_str ());
	 	}
		emit updateBoxColor ("Violet", "Other");
		emit notification ("Loading Experiment Settings...\n");
		ConfigStream cStream (input->profile.configFilePath (), true);
		loadExperimentRuntime (cStream, expRuntime);
		if (input->expType != ExperimentType::LoadMot) {
			emit notification ("Loading Master Runtime...\n", 1);
			input->logger.logMasterRuntime (expRuntime);
		}
		for (auto& device : input->devices.list) {
			deviceLoadExpSettings (device, cStream);/*TODO: remove dds from device, and now device only has andor*/
		}
		input->numVariations = determineVariationNumber(expRuntime.expParams);

		/// The Variation Calculation Step.
		emit notification ("Calculating All Variation Data...\r\n");
		for (auto& device : input->devices.list) {
			deviceCalculateVariations (device, expRuntime.expParams);
		}
		calculateAdoVariations (expRuntime);

		/// In-Experiment calibration
		calibrationOptionReport(expRuntime);
		inExpCalibrationProcedure(expRuntime, true/*false*/);
		emit startInExpCalibrationTimer();

		/// Anaylsis preparation
		emit prepareAnalysis();
		emit notification("Enabling real time analysis \r\n", 1);

		runConsistencyChecks (expRuntime.expParams, input->calibrations);
		if (input->expType != ExperimentType::LoadMot) {
			for (auto& device : input->devices.list) {
				if (device.get ().experimentActive) {
					emit notification (qstr ("Logging Device " + device.get ().getDelim ()
						+ " Settings...\n"), 1);
					device.get ().logSettings (input->logger, this);
				}
			}
		}

		/// Begin experiment 
		std::vector<double> finaltimes = input->ttls.getFinalTimes();

		if (expRuntime.mainOpts.repetitionFirst) {
			emit notification("Experiment programmed to be running " + qstr(expRuntime.repetitions) +
				" repetition first and then run it for all " + qstr(determineVariationNumber(expRuntime.expParams)) + " variations \r\n");

			for (const auto& variationInc : range(determineVariationNumber(expRuntime.expParams))) {
				initVariation(variationInc, expRuntime.expParams);
				emit notification("Programming Devices for Variation...#" + qstr(variationInc) + "\n");
				for (auto& device : input->devices.list) {
					deviceProgramVariation(device, expRuntime.expParams, variationInc);
				}
				emit notification("Running Experiment.\n");
				for (const auto& repInc : range(expRuntime.repetitions)) {
					inExpCalibrationRun(expRuntime);
					emit notification(qstr("Starting Repetition #" + qstr(repInc) + "\n"), 2);
					handlePause(isPaused, isAborting);
					startRep(repInc, variationInc, input->skipNext == nullptr ? false : input->skipNext->load());
					waitForSequenceFinish(finaltimes[variationInc]);
				}
			}
		}
		else {
			emit notification("Experiment programmed to be running " + qstr(determineVariationNumber(expRuntime.expParams)) +
				" variation first and then repeat it for " + qstr(expRuntime.repetitions) + " repetitions \r\n");

			for (const auto& repInc : range(expRuntime.repetitions)) {
				emit notification(qstr("Starting Repetition #" + qstr(repInc) + "\n"), 0);
				emit repUpdate(repInc);
				for (const auto& variationInc : range(determineVariationNumber(expRuntime.expParams))) {
					inExpCalibrationRun(expRuntime);
					emit notification("Programming Devices for Variation...\n", 2);
					qDebug() << "Programming Devices for Variation"<< variationInc;
					for (auto& device : input->devices.list) {
						deviceProgramVariation(device, expRuntime.expParams, variationInc);
					}
					initVariation(variationInc, expRuntime.expParams);
					handlePause(isPaused, isAborting);
					startRep(repInc, variationInc, input->skipNext == nullptr ? false : input->skipNext->load());
					waitForSequenceFinish(finaltimes[variationInc]);
				}
			}
		}



		waitForAndorFinish ();
		for (auto& device : input->devices.list) {
			deviceNormalFinish (device);
		}
		normalFinish (input->expType, true /*runmaster*/, startTime);
	}
	catch (ChimeraError & exception) {
		for (auto& device : input->devices.list) {
			// don't change the colors, the colors should reflect the end state before the error. 
			if (device.get ().experimentActive) {
				device.get ().errorFinish ();
			}
		}
		errorFinish (isAborting, exception, startTime);
	}
	experimentIsRunning = false;
}


void ExpThreadWorker::analyzeMasterScript (DoCore& ttls, AoCore& ao, DdsCore& dds, OlCore& ol,
	std::vector<parameterType>& vars,
	ScriptStream& currentMasterScript, bool expectsLoadSkip,
	std::string& warnings, timeType& operationTime,
	timeType& loadSkipTime, repeatManager& repeatMgr)
{
	std::string currentMasterScriptText = currentMasterScript.str ();
	loadSkipTime.first.clear ();
	loadSkipTime.second = 0.0;
	// starts at 0.1 if not initialized by the user.
	operationTime.second = 0.1;
	operationTime.first.clear ();
	if (currentMasterScript.str () == "") {
		thrower ("Master script is empty! (A low level bug, this shouldn't happen)");
	}
	std::string word;
	currentMasterScript >> word;
	// the analysis loop.
	bool loadSkipFound = false;
	std::string scope = PARENT_PARAMETER_SCOPE;
	try {
		while (!(currentMasterScript.peek () == EOF) || word != "__end__") {
			if (handleTimeCommands (word, currentMasterScript, vars, scope, operationTime, repeatMgr)) {
				// got handled, so break out of the if-else by entering this scope.
			}
			else if (handleVariableDeclaration (word, currentMasterScript, vars, scope, warnings)) {}
			else if (handleDoCommands (word, currentMasterScript, vars, ttls, scope, operationTime, repeatMgr)) {}
			else if (handleAoCommands (word, currentMasterScript, vars, ao, ttls, scope, operationTime, repeatMgr)) {}
			else if (handleDdsCommands(word, currentMasterScript, vars, dds, scope, operationTime, repeatMgr)) {}
			else if (handleOlCommands(word, currentMasterScript, vars, ol, scope, operationTime, repeatMgr)) {}
			else if (handleRepeats(word, currentMasterScript, vars, ttls, ao, dds, ol, scope, repeatMgr)) {}
			else if (word == "callcppcode") {
				// and that's it... 
				callCppCodeFunction ();
			}
			else if (word == "loadskipentrypoint!") {
				loadSkipFound = true;
				loadSkipTime = operationTime;
			}
			/// Deal with RSG calls
			else if (word == "rsg:") {
				thrower ("\"rsg:\" command is deprecated! Please use the microwave system listview instead.");
			}
			else if (handleFunctionCall (word, currentMasterScript, vars, ttls, ao, dds, ol, warnings,
				PARENT_PARAMETER_SCOPE, operationTime, repeatMgr)) {
			}
			else {
				word = (word == "") ? "[EMPTY-STRING]" : word;
				thrower ("unrecognized master script command: \"" + word + "\" around \r\n" + 
					currentMasterScript.getline());
			}
			word = "";
			currentMasterScript >> word;
		}
	}
	catch (ChimeraError&) {
		throwNested ("Error Seen While Analyzing Master Script!");
	}
	if (expectsLoadSkip && !loadSkipFound) {
		thrower ("Expected load skip in script, but the load skip command was not found during script analysis!");
	}
}


void ExpThreadWorker::analyzeFunction (std::string function, std::vector<std::string> args, DoCore& ttls, 
	AoCore& ao, DdsCore& dds, OlCore& ol, std::vector<parameterType>& params, std::string& warnings, timeType& operationTime,
	std::string callingScope, repeatManager& repeatMgr) {
	/// load the file
	std::fstream functionFile;
	// check if file address is good.
	FILE* file;
	fopen_s (&file, cstr (FUNCTIONS_FOLDER_LOCATION + function + "." + Script::FUNCTION_EXTENSION), "r");
	if (!file) {
		thrower ("Function " + function + " does not exist! The master script tried to open this function, it"
			" tried and failed to open the location " + FUNCTIONS_FOLDER_LOCATION + function + "."
			+ Script::FUNCTION_EXTENSION + ".");
	}
	else {
		fclose (file);
	}
	functionFile.open (FUNCTIONS_FOLDER_LOCATION + function + "." + Script::FUNCTION_EXTENSION, std::ios::in);
	// check opened correctly
	if (!functionFile.is_open ()) {
		thrower ("Function file " + function + "File passed test making sure the file exists, but it still "
			"failed to open! (A low level bug, this shouldn't happen.)");
	}
	// append __END__ to the end of the file for analysis purposes.
	std::stringstream buf;
	ScriptStream functionStream;
	buf << functionFile.rdbuf ();
	functionStream << buf.str ();
	functionStream << "\r\n\r\n__END__";
	functionFile.close ();
	if (functionStream.str () == "") {
		thrower ("Function File for " + function + " function was empty! (A low level bug, this shouldn't happen");
	}
	std::string word;
	std::string scope = function;
	/// get the function arguments.
	std::string defLine, name;
	defLine = functionStream.getline (':');
	std::vector<std::string> functionArgs;
	analyzeFunctionDefinition (defLine, name, functionArgs);
	if (functionArgs.size () != args.size ()) {
		std::string functionArgsString;
		for (auto elem : args) {
			functionArgsString += elem + ",";
		}
		thrower ("incorrect number of arguments in the call for function " + function + ". Number in call was: "
			+ str (args.size ()) + ", number expected was " + str (functionArgs.size ())
			+ ". Function arguments were:" + functionArgsString + ".");
	}
	std::vector<std::pair<std::string, std::string>> replacements;
	for (auto replacementInc : range (args.size ())) {
		replacements.push_back ({ functionArgs[replacementInc], args[replacementInc] });
	}
	functionStream.loadReplacements (replacements, params, function, callingScope, function);
	std::string currentFunctionText = functionStream.str ();
	///
	functionStream >> word;
	try {
		while (!(functionStream.peek () == EOF) || word != "__end__") {
			if (handleTimeCommands(word, functionStream, params, scope, operationTime, repeatMgr)) { /* got handled*/ }
			else if (handleVariableDeclaration(word, functionStream, params, scope, warnings)) {}
			else if (handleDoCommands(word, functionStream, params, ttls, scope, operationTime, repeatMgr)) {}
			else if (handleAoCommands(word, functionStream, params, ao, ttls, scope, operationTime, repeatMgr)) {}
			else if (handleDdsCommands(word, functionStream, params, dds, scope, operationTime, repeatMgr)) {}
			else if (handleOlCommands(word, functionStream, params, ol, scope, operationTime, repeatMgr)) {}
			else if (handleRepeats(word, functionStream, params, ttls, ao, dds, ol, scope, repeatMgr)) {}
			else if (word == "callcppcode") {
				// and that's it... 
				callCppCodeFunction ();
			}
			/// Handle RSG calls.
			else if (word == "rsg:") {
				thrower ("\"rsg:\" command is deprecated! Please use the microwave system listview instead.");
			}
			/// deal with function calls.
			else if (handleFunctionCall (word, functionStream, params, ttls, ao, dds, ol, warnings, function, operationTime, repeatMgr)) {}
			else {
				thrower ("unrecognized master script command inside function analysis: " + word);
			}
			functionStream >> word;
		}
	}
	catch (ChimeraError&) {
		throwNested ("Failed to analyze function \"" + function + "\"");
	}
}


bool ExpThreadWorker::getAbortStatus () {
	return isAborting;
}

double ExpThreadWorker::convertToTime (timeType time, std::vector<parameterType> variables, unsigned variation) {
	double variableTime = 0;
	// add together current values for all variable times.
	if (time.first.size () != 0) {
		for (auto varTime : time.first) {
			variableTime += varTime.evaluate (variables, variation);
		}
	}
	return variableTime + time.second;
}

void ExpThreadWorker::handleDebugPlots (DoCore& ttls, AoCore& aoSys, OlCore& olSys, unsigned variation) {
	emit doAoOlData(ttls.getPlotData (variation), aoSys.getPlotData (variation), olSys.getPlotData(variation));
	//emit notification (qstr (ttls.getTtlSequenceMessage (variation)), 2);
	//emit notification (qstr (aoSys.getDacSequenceMessage (variation)), 2);
}

bool ExpThreadWorker::runningStatus () {
	return experimentIsRunning;
}

void ExpThreadWorker::startExperimentThread (ExperimentThreadInput* input, IChimeraQtWindow* parent) {
}

bool ExpThreadWorker::getIsPaused () {
	return isPaused;
}

void ExpThreadWorker::pause () {
	if (!experimentIsRunning) {
		thrower ("Can't pause the experiment if the experiment isn't running!");
	}
	isPaused = true;
}

void ExpThreadWorker::unPause () {
	if (!experimentIsRunning) {
		thrower ("Can't unpause the experiment if the experiment isn't running!");
	}
	isPaused = false;
}

void ExpThreadWorker::abort () {
	if (!experimentIsRunning) {
		thrower ("Can't abort the experiment if the experiment isn't running!");
	}
	isAborting = true;
}

void ExpThreadWorker::loadGMoogScript(std::string scriptAddress, ScriptStream& gmoogScript) 
{
	std::ifstream scriptFile(scriptAddress);
	if (!scriptFile.is_open()) {
		thrower("Scripted GigaMoog File \"" + scriptAddress + "\" failed to open!");
	}
	gmoogScript << scriptFile.rdbuf();
	gmoogScript << "\r\n\r\n__END__";
	gmoogScript.seekg(0);
	scriptFile.close();
}

void ExpThreadWorker::loadArbGenScript(std::string scriptAddress, ScriptStream& agilentScript) {
	std::ifstream scriptFile (scriptAddress);
	if (!scriptFile.is_open ()) {
		thrower ("Scripted Agilent File \"" + scriptAddress + "\" failed to open!");
	}
	agilentScript << scriptFile.rdbuf ();
	agilentScript.seekg (0);
	scriptFile.close ();
}

void ExpThreadWorker::loadMasterScript (std::string scriptAddress, ScriptStream& currentMasterScript) {
	std::ifstream scriptFile;
	// check if file address is good.
	FILE* file;
	fopen_s (&file, cstr (scriptAddress), "r");
	if (!file) {
		thrower ("The Master Script File " + scriptAddress + " does not exist! The Master-Manager tried to "
			"open this file before starting the script analysis.");
	}
	else {
		fclose (file);
	}
	scriptFile.open (cstr (scriptAddress));
	// check opened correctly
	if (!scriptFile.is_open ()) {
		thrower ("File passed test making sure the file exists, but it still failed to open?!?! "
			"(A low level-bug, this shouldn't happen.)");
	}
	// dump the file into the stringstream.
	std::stringstream buf (std::ios_base::app | std::ios_base::out | std::ios_base::in);
	// IMPORTANT!

	//buf << "\r\n t = 0.01 \r\n pulseon: " + str (OSCILLOSCOPE_TRIGGER) + " 0.02\r\n t += 0.1\r\n";
	buf << scriptFile.rdbuf ();
	//// this is used to more easily deal some of the analysis of the script.
	buf << "\r\n\r\n__END__";

	// for whatever reason, after loading rdbuf into a stringstream, the stream seems to not 
	// want to >> into a string. tried resetting too using seekg, but whatever, this works.
	currentMasterScript.str ("");
	currentMasterScript.str (buf.str ());
	currentMasterScript.clear ();
	currentMasterScript.seekg (0);
	scriptFile.close ();
}

// makes sure formatting is correct, returns the arguments and the function name from reading the firs real line of a function file.
void ExpThreadWorker::analyzeFunctionDefinition (std::string defLine, std::string& functionName,
												 std::vector<std::string>& args) {
	args.clear ();
	ScriptStream defStream (defLine);
	std::string word;
	defStream >> word;
	if (word == "") {
		defStream >> word;
	}
	if (word != "def") {
		thrower ("Function file (extenion \".func\") in functions folder was not a function because it did not"
			" start with \"def\"! Functions must start with this. Instead it started with \"" + word + "\".");
	}
	std::string functionDeclaration, functionArgumentList;
	functionDeclaration = defStream.getline (':');
	int initNamePos = defLine.find_first_not_of (" \t");
	functionName = functionDeclaration.substr (initNamePos, functionDeclaration.find_first_of ("(") - initNamePos);

	if (functionName.find_first_of (" ") != std::string::npos) {
		thrower ("Function name included a space!");
	}
	int initPos = functionDeclaration.find_first_of ("(");
	if (initPos == std::string::npos) {
		thrower ("No starting parenthesis \"(\" in function definition. Use \"()\" if no arguments.");
	}
	initPos++;
	int endPos = functionDeclaration.find_last_of (")");
	if (endPos == std::string::npos) {
		thrower ("No ending parenthesis \")\" in function definition. Use \"()\" if no arguments.");
	}
	functionArgumentList = functionDeclaration.substr (initPos, endPos - initPos);
	endPos = functionArgumentList.find_first_of (",");
	initPos = functionArgumentList.find_first_not_of (" \t");
	bool good = true;
	// fill out args.
	while (initPos != std::string::npos) {
		// get initial argument
		std::string tempArg = functionArgumentList.substr (initPos, endPos - initPos);
		if (endPos == std::string::npos) {
			functionArgumentList = "";
		}
		else {
			functionArgumentList.erase (0, endPos + 1);
		}
		// clean up any spaces on beginning and end.
		int lastChar = tempArg.find_last_not_of (" \t");
		int lastSpace = tempArg.find_last_of (" \t");
		if (lastSpace > lastChar){
			tempArg = tempArg.substr (0, lastChar + 1);
		}
		// now it should be clean. Check if there are spaces in the middle.
		if (tempArg.find_first_of (" \t") != std::string::npos){
			thrower ("bad argument list in function. It looks like there might have been a space or tab inside "
				"the function argument? (A low level bug, this shouldn't happen.)");
		}
		if (tempArg == ""){
			thrower ("bad argument list in function. It looks like there might have been a stray \",\"?");
		}
		args.push_back (tempArg);
		endPos = functionArgumentList.find_first_of (",");
		initPos = functionArgumentList.find_first_not_of (" \t");
	}
}

// at least right now, this doesn't support varying any of the values of the constant vector. this could probably
// be sensibly changed at some point.
bool ExpThreadWorker::handleVectorizedValsDeclaration (std::string word, ScriptStream& stream,
	std::vector<vectorizedNiawgVals>& constVecs, std::string& warnings) {
	if (word != "var_v") {
		return false;
	}
	std::string vecLength;
	vectorizedNiawgVals tmpVec;
	stream >> vecLength >> tmpVec.name;
	for (auto& cv : constVecs) {
		if (tmpVec.name == cv.name) {
			thrower ("Constant Vector name \"" + tmpVec.name + "\"being re-used! You may only declare one constant "
				"vector with this name.");
		}
	}
	unsigned vecLength_ui = 0;
	try {
		vecLength_ui = boost::lexical_cast<unsigned>(vecLength);
	}
	catch (boost::bad_lexical_cast) {
		thrower ("Failed to convert constant vector length to an unsigned int!");
	}
	if (vecLength_ui == 0 || vecLength_ui > 32) {
		thrower ("Invalid constant vector length: " + str (vecLength_ui) + ", must be greater than 0 and less than "
			+ str (32));
	}
	std::string bracketDelims;
	stream >> bracketDelims;
	if (bracketDelims != "[") {
		thrower ("Expected \"[\" after constant vector size and name (make sure it's separated by spaces).");
	}
	tmpVec.vals.resize (vecLength_ui);
	for (auto& val : tmpVec.vals) {
		stream >> val;
	}
	stream >> bracketDelims;
	if (bracketDelims != "]") {
		thrower ("Expected \"]\" after constant vector values (make sure it's separated by spaces). Is the vector size right?");
	}
	constVecs.push_back (tmpVec);
	return true;
}

std::vector<parameterType> ExpThreadWorker::getLocalParameters (ScriptStream& stream) {
	std::string scriptText = stream.str ();
	if (scriptText == "") {
		return {};
	}
	std::string word;
	stream >> word;
	// the analysis loop.
	std::vector<parameterType> params;
	std::vector<vectorizedNiawgVals> niawgParams;
	std::string warnings = "";
	while (!(stream.peek () == EOF) && !stream.eof () && word != "__end__") {
		auto peekpos = stream.peek ();
		try {
			if (handleVariableDeclaration (word, stream, params, GLOBAL_PARAMETER_SCOPE, warnings)) {
			}
			else (handleVectorizedValsDeclaration (word, stream, niawgParams, warnings));
		}
		catch (ChimeraError&) { /*Easy for this to happen. */ }
		word = "";
		stream >> word;
	}
	for (auto& param : niawgParams) {
		parameterType temp;
		temp.name = param.name;
		params.push_back (temp);
	}
	return params;
}

bool ExpThreadWorker::handleVariableDeclaration (std::string word, ScriptStream& stream, std::vector<parameterType>& vars,
	std::string scope, std::string& warnings) {
	if (word != "var") {
		return false;
	}
	// add to variables!
	std::string name, valStr;
	stream >> name >> valStr;
	parameterType tmpVariable;
	tmpVariable.constant = true;
	std::transform (name.begin (), name.end (), name.begin (), ::tolower);
	tmpVariable.name = name;
	for (auto var : vars) {
		if (var.name == tmpVariable.name) {
			if (var.parameterScope == GLOBAL_PARAMETER_SCOPE) {
				warnings += "Warning: local variable \"" + tmpVariable.name + "\" with scope \""
					+ scope + "\"is being overwritten by a parameter with the same name and "
					"global parameter scope. It is generally recommended to use the appropriate local scope when "
					"possible.\r\n";
				// this variable is being overwritten, so don't add this variable vector
				return true;
			}
			else if (str (var.parameterScope, 13, false, true) == str (scope, 13, false, true)) {
				// being overwritten so don't add, but the variable was specific, so this must be fine.
				return true;
			}
		}
	}
	bool found = false;
	double val;
	try {
		val = boost::lexical_cast<double>(valStr);
	}
	catch (boost::bad_lexical_cast&) {
		throwNested ("Bad string for value of local variable " + str (name));
	}
	tmpVariable.constantValue = val;
	tmpVariable.scanDimension = 1;
	tmpVariable.parameterScope = str (scope, 13, false, true);
	tmpVariable.ranges.push_back ({ val, val });
	// these are always constants, so just go ahead and set the keyvalue for use manually. 
	if (vars.size () == 0) {
		tmpVariable.keyValues = std::vector<double> (1, val);
	}
	else {
		tmpVariable.keyValues = std::vector<double> (vars.front ().keyValues.size (), val);
	}
	vars.push_back (tmpVariable);
	return true;
}


// if it handled it, returns true, else returns false.
bool ExpThreadWorker::handleTimeCommands (std::string word, ScriptStream& stream, std::vector<parameterType>& vars,
	std::string scope, timeType& operationTime, repeatManager& repeatMgr) {
	bool repeating = repeatMgr.isRepeating();
	try {
		if (word == "t") {
			std::string command;
			stream >> command;
			word += command;
		}
		//
		if (word == "t++") {
			operationTime.second++;
			if (repeating) {
				repeatMgr.getCurrentActiveItem()->data().repeatAddedTime.second++;
			}
		}
		else if (word == "t+=") {
			Expression time;
			stream >> time;
			try {
				operationTime.second += time.evaluate ();
				if (repeating) {
					repeatMgr.getCurrentActiveItem()->data().repeatAddedTime.second += time.evaluate();
				}
			}
			catch (ChimeraError&) {
				time.assertValid (vars, scope);
				operationTime.first.push_back (time);
				if (repeating) {
					repeatMgr.getCurrentActiveItem()->data().repeatAddedTime.first.push_back(time);
				}
			}
		}
		else if (word == "t=") {
			if (repeating) {
				throwNested("Can not use \"t=\" syntax inside a repeat. Use incremental syntax instead. This would cause the sequence time to be reset during repeat,"
					"which may be what you want but I am pretty sure there are other way to achieve the same result without using \"t=\" inside repeat.");
			}
			if (repeatMgr.repeatHappend()) {
				throwNested("Can not use \"t=\" syntax after a repeat. Use incremental syntax instead. This would cause the sequence time to be reset after repeat,"
					"and will definitively get affected by the repeat. A possible fix is to break the time command according to \"t=\", so that "
					"each block of time can be analyzed separately. But this would require to change all Do,Ao,Dds,Ol's CommandFormList to vec of vec for different time commnad segment. ");
			}
			Expression time;
			stream >> time;
			try {
				operationTime.first.clear();
				operationTime.second = time.evaluate ();
			}
			catch (ChimeraError&) {
				time.assertValid (vars, scope);
				operationTime.first.clear();
				operationTime.first.push_back (time);
				// because it's equals. There shouldn't be any extra terms added to this now.
				operationTime.second = 0;
			}
		}
		else {
			return false;
		}
		return true;
	}
	catch (ChimeraError&) {
		throwNested ("Error seen while handling time commands. Word was \"" + word + "\"");
	}
}

/* returns true if handles word, false otherwise. */
bool ExpThreadWorker::handleDoCommands (std::string word, ScriptStream& stream, std::vector<parameterType>& vars,
	DoCore& ttls, std::string scope, timeType& operationTime, repeatManager& repeatMgr) {
	repeatInfoId repeatId = repeatMgr.getCurrentActiveID();
	if (word == "on:" || word == "off:") {
		std::string name;
		stream >> name;
		ttls.handleTtlScriptCommand (word, operationTime, name, vars, scope, repeatId);
	}
	else if (word == "pulseon:" || word == "pulseoff:") {
		// this requires handling time as it is handled above.
		std::string name;
		Expression pulseLength;
		stream >> name >> pulseLength;
		ttls.handleTtlScriptCommand (word, operationTime, name, pulseLength, vars, scope, repeatId);
	}
	else {
		return false;
	}
	return true;
}

/* returns true if handles word, false otherwise. */
bool ExpThreadWorker::handleAoCommands (std::string word, ScriptStream& stream,	std::vector<parameterType>& params, AoCore& ao, DoCore& ttls,
	std::string scope, timeType& operationTime, repeatManager& repeatMgr) {
	repeatInfoId repeatId = repeatMgr.getCurrentActiveID();
	if (word == "cao:") {
		// zzp 10/28/2022 - I think this is not used in any of B232's code and we can deprecate this keyword
		// for calibrated dac output, the syntax should be 
		// dac: dac_name { dac_value cal_name }

		// calibrated analog out. Right now, the code doesn't having a variable calibration setting, as this would
		// require the calibration to... do something funny. 
		AoCommandForm command;
		std::string dacName, calibrationName, calibratedVal;
		stream >> calibrationName >> dacName >> calibratedVal;
		command.finalVal = calibratedVal;
		command.finalVal.assertValid (params, scope);
		command.time = operationTime;
		command.commandName = "dac:";
		command.numSteps.expressionStr = command.initVal.expressionStr = "__NONE__";
		command.rampTime.expressionStr = command.rampInc.expressionStr = "__NONE__";
		command.repeatId = repeatId;
		try {
			ao.handleDacScriptCommand (command, dacName, params);
		}
		catch (ChimeraError&) {
			throwNested ("Error handling \"cao:\" command.");
		}
	}
	if (word == "dac:") {
		AoCommandForm command;
		std::string name;
		stream >> name >> command.finalVal;
		command.finalVal.assertValid (params, scope);
		command.time = operationTime;
		command.commandName = "dac:";
		command.numSteps.expressionStr = command.initVal.expressionStr = "__NONE__";
		command.rampTime.expressionStr = command.rampInc.expressionStr = "__NONE__";
		command.repeatId = repeatId;
		try {
			ao.handleDacScriptCommand (command, name, params);
		}
		catch (ChimeraError&) {
			throwNested ("Error handling \"dac:\" command.");
		}
	}
	else if (word == "daclinspace:") 
	{
		AoCommandForm command;
		std::string name;
		stream >> name >> command.initVal >> command.finalVal >> command.rampTime >> command.numSteps;
		command.initVal.assertValid (params, scope);
		command.finalVal.assertValid (params, scope);
		command.rampTime.assertValid (params, scope);
		command.numSteps.assertValid (params, scope);
		command.time = operationTime;
		command.commandName = "daclinspace:";
		command.repeatId = repeatId;
		// not used here.
		command.rampInc.expressionStr = "__NONE__";
		//
		try {
			ao.handleDacScriptCommand (command, name, params);
		}
		catch (ChimeraError&) {
			throwNested ("Error handling \"dacLinSpace:\" command.");
		}
	}
	else if (word == "dacarange:")
	{
		AoCommandForm command;
		std::string name;
		stream >> name >> command.initVal >> command.finalVal >> command.rampTime >> command.rampInc;
		command.initVal.assertValid (params, scope);
		command.finalVal.assertValid (params, scope);
		command.rampTime.assertValid (params, scope);
		command.rampInc.assertValid (params, scope);
		command.time = operationTime;
		command.commandName = "dacarange:";
		// not used here.
		command.numSteps.expressionStr = "__NONE__";
		command.repeatId = repeatId;
		try {
			ao.handleDacScriptCommand (command, name, params);
		}
		catch (ChimeraError&) {
			throwNested ("Error handling \"dacArange:\" command.");
		}
	}
	else if (word == "dacramp:")
	{
		AoCommandForm command;
		std::string name;
		stream >> name >> command.initVal >> command.finalVal >> command.rampTime;
		command.initVal.assertValid(params, scope);
		command.finalVal.assertValid(params, scope);
		command.rampTime.assertValid(params, scope);
		command.time = operationTime;
		command.commandName = "dacramp:";
		// not used here. 
		command.numSteps.expressionStr = "__NONE__";
		command.rampInc.expressionStr = "__NONE__";
		command.repeatId = repeatId;
		try
		{
			ao.handleDacScriptCommand(command, name, params);
		}
		catch (ChimeraError& err)
		{
			throwNested("Error handling in \"dacramp:\" command inside main script");
		}
	}
	else
	{
		return false;
	}
	return true;
}

/* returns true if handles word, false otherwise. */
bool ExpThreadWorker::handleDdsCommands(std::string word, ScriptStream& stream, std::vector<parameterType>& vars,
	DdsCore& ddss, std::string scope, timeType& operationTime, repeatManager& repeatMgr)
{
	repeatInfoId repeatId = repeatMgr.getCurrentActiveID();
	if (word == "ddsamp:") //ddsamp: name amp
	{
		DdsCommandForm command;
		std::string name;
		stream >> name >> command.initVal;
		command.initVal.assertValid(vars, scope);
		command.time = operationTime;
		command.commandName = "ddsamp:";
		command.finalVal.expressionStr = "__NONE__";
		command.rampTime.expressionStr = "__NONE__";
		command.numSteps.expressionStr = "__NONE__";
		command.repeatId = repeatId;
		try
		{
			ddss.handleDDSScriptCommand(command, name, vars);
		}
		catch (ChimeraError& err)
		{
			throwNested("Error handling \"ddsamp:\" command inside main script");
		}
	}
	else if (word == "ddsfreq:") //ddsfreq: name freq
	{
		DdsCommandForm command;
		std::string name;
		stream >> name >> command.initVal;
		command.initVal.assertValid(vars, scope);
		command.time = operationTime;
		command.commandName = "ddsfreq:";
		command.finalVal.expressionStr = "__NONE__";
		command.rampTime.expressionStr = "__NONE__";
		command.numSteps.expressionStr = "__NONE__";
		command.repeatId = repeatId;
		try
		{
			ddss.handleDDSScriptCommand(command, name, vars);
		}
		catch (ChimeraError& err)
		{
			throwNested("Error handling \"ddsfreq:\" command inside main script");
		}
	}
	else if (word == "ddslinspaceamp:") //ddslinspaceamp: name initAmp finalAmp rampTime numSteps
	{
		DdsCommandForm command;
		std::string name;
		stream >> name >> command.initVal >> command.finalVal >> command.rampTime >> command.numSteps;
		command.initVal.assertValid(vars, scope);
		command.finalVal.assertValid(vars, scope);
		command.rampTime.assertValid(vars, scope);
		command.numSteps.assertValid(vars, scope);
		command.time = operationTime;
		command.commandName = "ddslinspaceamp:";
		command.repeatId = repeatId;
		try
		{
			ddss.handleDDSScriptCommand(command, name, vars);
		}
		catch (ChimeraError& err)
		{
			throwNested("Error handling \"ddslinspaceamp:\" command inside main script");
		}
	}
	else if (word == "ddslinspacefreq:")  //ddslinspacefreq: name initFreq finalFreq rampTime numSteps
	{
		DdsCommandForm command;
		std::string name;
		stream >> name >> command.initVal >> command.finalVal >> command.rampTime >> command.numSteps;
		command.initVal.assertValid(vars, scope);
		command.finalVal.assertValid(vars, scope);
		command.rampTime.assertValid(vars, scope);
		command.numSteps.assertValid(vars, scope);
		command.time = operationTime;
		command.commandName = "ddslinspacefreq:";
		command.repeatId = repeatId;
		try
		{
			ddss.handleDDSScriptCommand(command, name, vars);
		}
		catch (ChimeraError& err)
		{
			throwNested("Error handling \"ddslinspacefreq:\" command inside main script");
		}
	}
	else if (word == "ddsrampamp:") // ddsrampamp: name intiAmp finalAmp rampTime
	{
		DdsCommandForm command;
		std::string name;
		stream >> name >> command.initVal >> command.finalVal >> command.rampTime;
		command.initVal.assertValid(vars, scope);
		command.finalVal.assertValid(vars, scope);
		command.rampTime.assertValid(vars, scope);
		command.time = operationTime;
		command.commandName = "ddsrampamp:";
		command.numSteps.expressionStr = "__NONE__";
		command.repeatId = repeatId;
		try
		{
			ddss.handleDDSScriptCommand(command, name, vars);
		}
		catch (ChimeraError& err)
		{
			throwNested("Error handling \"ddsrampamp:\" command inside main script");
		}
	}
	else if (word == "ddsrampfreq:") // ddsrampfreq: name intiFreq finalFreq rampTime
	{
		DdsCommandForm command;
		std::string name;
		stream >> name >> command.initVal >> command.finalVal >> command.rampTime;
		command.initVal.assertValid(vars, scope);
		command.finalVal.assertValid(vars, scope);
		command.rampTime.assertValid(vars, scope);
		command.time = operationTime;
		command.commandName = "ddsrampfreq:";
		command.numSteps.expressionStr = "__NONE__";
		command.repeatId = repeatId;
		try
		{
			ddss.handleDDSScriptCommand(command, name, vars);
		}
		catch (ChimeraError& err)
		{
			throwNested("Error handling \"ddsrampfreq:\" command inside main script");
		}
	}
	else
	{
		return false;
	}
	return true;
}

/* returns true if handles word, false otherwise. */
bool ExpThreadWorker::handleOlCommands(std::string word, ScriptStream& stream, std::vector<parameterType>& vars,
	OlCore& ols, std::string scope, timeType& operationTime, repeatManager& repeatMgr)
{
	repeatInfoId repeatId = repeatMgr.getCurrentActiveID();
	if (word == "ol:") //ddsamp: name amp
	{
		OlCommandForm command;
		std::string name;
		stream >> name >> command.initVal;
		command.initVal.assertValid(vars, scope);
		command.time = operationTime;
		command.commandName = "ol:";
		command.finalVal.expressionStr = "__NONE__";
		command.rampTime.expressionStr = "__NONE__";
		command.numSteps.expressionStr = "__NONE__";
		command.repeatId = repeatId;
		try
		{
			ols.handleOLScriptCommand(command, name, vars);
		}
		catch (ChimeraError& err)
		{
			throwNested("Error handling \"ol:\" command inside main script");
		}
	}
	else if (word == "olramp:")  //ddslinspacefreq: name initFreq finalFreq rampTime numSteps
	{
		OlCommandForm command;
		std::string name;
		stream >> name >> command.initVal >> command.finalVal >> command.rampTime /*>> command.numSteps*/;
		command.initVal.assertValid(vars, scope);
		command.finalVal.assertValid(vars, scope);
		command.rampTime.assertValid(vars, scope);
		//command.numSteps.assertValid(vars, scope);
		command.time = operationTime;
		command.commandName = "olramp:";
		command.repeatId = repeatId;
		try
		{
			ols.handleOLScriptCommand(command, name, vars);
		}
		catch (ChimeraError& err)
		{
			throwNested("Error handling \"olramp:\" command inside main script");
		}
	}
	else if (word == "ollinspace:") {
		OlCommandForm command;
		std::string name;
		stream >> name >> command.initVal >> command.finalVal >> command.rampTime >> command.numSteps;
		command.initVal.assertValid(vars, scope);
		command.finalVal.assertValid(vars, scope);
		command.rampTime.assertValid(vars, scope);
		command.numSteps.assertValid(vars, scope);
		command.time = operationTime;
		command.commandName = "ollinspace:";
		command.repeatId = repeatId;
		// not used here.
		command.rampInc.expressionStr = "__NONE__";
		//
		try {
			ols.handleOLScriptCommand(command, name, vars);
		}
		catch (ChimeraError&) {
			throwNested("Error handling \"ollinspace:\" command.");
		}
	}
	else
	{
		return false;
	}
	return true;
}

bool ExpThreadWorker::handleRepeats(std::string word, ScriptStream& stream, std::vector<parameterType>& params, 
	DoCore& ttls, AoCore& ao, DdsCore& dds, OlCore& ol,
	std::string scope, repeatManager& repeatMgr)
{
	if (word == "repeat:") {
		// handle start of repeat
		auto* child = repeatMgr.addNewRepeat();

		Expression repeatNum;
		stream >> repeatNum;
		repeatNum.assertValid(params, scope);
		child->data().repeatNum = repeatNum;
		child->data().repeatAddedTime = timeType(std::vector<Expression>(), 0.0);
	}
	else if (word == "end") {
		repeatInfoId repeatId = repeatMgr.getCurrentActiveID();
		if (!ttls.repeatsExistInCommandForm(repeatId)) {
			ttls.addPlaceholderRepeatCommand(repeatId);
		}
		if (!ao.repeatsExistInCommandForm(repeatId)) {
			ao.addPlaceholderRepeatCommand(repeatId);
		}
		if (!dds.repeatsExistInCommandForm(repeatId)) {
			dds.addPlaceholderRepeatCommand(repeatId);
		}
		if (!ol.repeatsExistInCommandForm(repeatId)) {
			ol.addPlaceholderRepeatCommand(repeatId);
		}
		// handle end of repeat
		repeatMgr.fininshCurrentRepeat();
	}
	else {
		return false;
	}
	return true;
}



/*
	this function can be called directly from scripts. Insert things inside the function to make it do something
	custom that's not possible inside the scripting language.
*/
void ExpThreadWorker::callCppCodeFunction () {
	// not used at the moment
}


bool ExpThreadWorker::isValidWord (std::string word) {
	if (word == "t" || word == "t++" || word == "t+=" || word == "t=" || word == "on:" || word == "off:"
		|| word == "dac:" || word == "dacarange:" || word == "daclinspace:" || word == "dacramp:" || word == "call"
		|| word == "repeat:" || word == "end" || word == "pulseon:" || word == "pulseoff:" || word == "callcppcode") {
		return true;
	}
	return false;
}

unsigned ExpThreadWorker::determineVariationNumber (std::vector<parameterType> variables) {
	int variationNumber;
	if (variables.size () == 0) {
		variationNumber = 1;
	}
	else {
		variationNumber = variables.front ().keyValues.size ();
		if (variationNumber == 0) {
			variationNumber = 1;
		}
	}
	return variationNumber;
}

void ExpThreadWorker::checkTriggerNumbers (std::vector<parameterType>& expParams) {
	/// check all trigger numbers between the DO system and the individual subsystems. These should almost always match,
	/// a mismatch is usually user error in writing the script.
	/// 
	emit notification("Running consistency checks for various experiment-active devices", 1);
	bool rsgMismatch = false;
	for (auto variationInc : range (determineVariationNumber (expParams))) {
		if (true /*runMaster*/) {
			auto& andorCamera = input->devices.getSingleDevice<AndorCameraCore>();
			if (andorCamera.experimentActive) {
				// check if there is just enough trigger for andor if it is used in the experiment
				if (variationInc == 0) {
					emit notification("Running consistency checks for Andor Camera", 2);
				}
				unsigned actualTrigs = input->ttls.countTriggers(ANDOR_TRIGGER_LINE, variationInc);
				unsigned expectedTrigs = andorCamera.getAndorRunSettings().picsPerRepetition;
				if (actualTrigs != expectedTrigs) {
					// this is a serious low-level/user error. throw, don't warn.
					std::string infoString = "Actual/Expected Andor Triggers: " + str(actualTrigs) + "/"
						+ str(expectedTrigs) + ".";
					thrower("The number of Andor triggers that the ttl system sends to the Andor camera does not "
						"match the number of images in the Andor control! " + infoString + ", seen in variation #"
						+ str(variationInc) + "\r\n");
				}
			}
			else {
				// check if there are triggers for andor but the andor is not set active. If so, warn.
				unsigned actualTrigs = input->ttls.countTriggers(ANDOR_TRIGGER_LINE, variationInc); 
				if (actualTrigs != 0 && variationInc == 0) {
					emit warn("There are " + qstr(actualTrigs) + " triggers sent to Andor trigger in ttl line ("
						+ qstr(ANDOR_TRIGGER_LINE.first) + "," + qstr(ANDOR_TRIGGER_LINE.second) + "), but the Andor system is not active." +
						"Make sure that this is what you actually want.\r\n", 0);
				}
			}

			auto makoCameras = input->devices.getDevicesByClass<MakoCameraCore>();
			for (auto makoCam : makoCameras) {
				if (makoCam.get().getRunningSettings().expActive) {
					if (variationInc == 0) {
						emit notification("Running consistency checks for Mako Camera: " + qstr(makoCam.get().CameraName()), 2);
					}
					auto triggerLine = makoCam.get().getTriggerLine();
					unsigned actualTrigs = input->ttls.countTriggers(triggerLine, variationInc);
					unsigned expectedTrigs = makoCam.get().getRunningSettings().picsPerRep;
					if (actualTrigs != expectedTrigs) {
						// this is a serious low-level/user error. throw, don't warn.
						std::string infoString = "Actual/Expected Mako Triggers: " + str(actualTrigs) + "/"
							+ str(expectedTrigs) + ".";
						thrower("The number of Mako triggers that the ttl system sends to the Mako camera does not "
							"match the number of images in the Mako control! " + infoString + ", seen in variation #"
							+ str(variationInc) + "\r\n");
					}
				}
				else {
					auto triggerLine = makoCam.get().getTriggerLine();
					unsigned actualTrigs = input->ttls.countTriggers(triggerLine, variationInc);
					if (actualTrigs != 0 && variationInc == 0) {
						emit warn("There are " + qstr(actualTrigs) + " triggers sent to MAKO trigger in ttl line ("
							+ qstr(triggerLine.first) + "," + qstr(triggerLine.second) + "), but the MAKO system: " + qstr(makoCam.get().CameraName())
							+ " is not active." + "Make sure that this is what you actually want.\r\n", 0);
					}
				}
			}

			auto& uwaveCore = input->devices.getSingleDevice<MicrowaveCore>();
			if (uwaveCore.experimentActive) {
				if (variationInc == 0) {
					emit notification("Running consistency checks for Microwave system: " + qstr(uwaveCore.queryIdentity()), 2);
				}
				auto actualTrigs = input->ttls.countTriggers(uwaveCore.getUWaveTriggerLine(), variationInc);
				auto expectedTrigs = uwaveCore.getNumTriggers(uwaveCore.experimentSettings);
				if (actualTrigs != expectedTrigs) {
					// this is a serious low-level/user error. throw, don't warn.
					std::string infoString = "Actual/Expected Microwave Triggers: " + str(actualTrigs) + "/"
						+ str(expectedTrigs) + ".";
					thrower("The number of Microwave triggers that the ttl system sends to the Microwave does not "
						"match the list size in microwave control! " + infoString + ", seen in variation #"
						+ str(variationInc) + "\r\n");
				}
			}
			else {
				auto triggerLine = uwaveCore.getUWaveTriggerLine();
				auto actualTrigs = input->ttls.countTriggers(triggerLine, variationInc);
				if (actualTrigs != 0 && variationInc == 0) {
					emit warn("There are " + qstr(actualTrigs) + " triggers sent to Microwave trigger in ttl line ("
						+ qstr(triggerLine.first) + "," + qstr(triggerLine.second) + "), but the Microwave system: " + qstr(uwaveCore.queryIdentity())
						+ " is not active." + "Make sure that this is what you actually want.\r\n", 0);
				}
			}
			
			/// check Agilents
			for (auto& arbGen : input->devices.getDevicesByClass<ArbGenCore>()) {
				arbGen.get().checkTriggers(variationInc, input->ttls, this);
			}

		}
	}
}

bool ExpThreadWorker::handleFunctionCall (std::string word, ScriptStream& stream, std::vector<parameterType>& vars,
	DoCore& ttls, AoCore& ao, DdsCore& dds, OlCore& ol, std::string& warnings,
	std::string callingFunction, timeType& operationTime, repeatManager& repeatMgr) {
	if (word != "call") {
		return false;
	}
	// calling a user-defined function. Get the name and the arguments to pass to the function handler.
	std::string functionCall, functionName, functionArgs;
	functionCall = stream.getline ('\n');
	//boost::erase_all (functionCall, "\r");
	int pos = functionCall.find_first_of ("(") + 1;
	int finalpos = functionCall.find_last_of (")");
	functionName = functionCall.substr (0, pos - 1);
	functionArgs = functionCall.substr (pos, finalpos - pos);
	std::string arg;
	std::vector<std::string> args;
	while (true) {
		pos = functionArgs.find_first_of (',');
		if (pos == std::string::npos) {
			arg = functionArgs.substr (0, functionArgs.size ());
			if (arg != "") {
				args.push_back (arg);
			}
			break;
		}
		arg = functionArgs.substr (0, pos);
		args.push_back (arg);
		// cut oinputut that argument off the string.
		functionArgs = functionArgs.substr (pos + 1, functionArgs.size ());
	}
	if (functionName == callingFunction) {
		thrower ("Recursive function call detected! " + callingFunction + " called itself! This is not allowed."
			" There is no way to end a function call conditionally so this will necessarily result in an"
			" infinite recursion\r\n");
	}
	try {
		analyzeFunction (functionName, args, ttls, ao, dds, ol, vars, warnings, operationTime, callingFunction, repeatMgr);
	}
	catch (ChimeraError&) {
		throwNested ("Error handling Function call to function " + functionName + ".");
	}
	return true;
}

void ExpThreadWorker::calculateAdoVariations (ExpRuntimeData& runtime) {
	if (true /*runMaster*/) {
		auto variations = determineVariationNumber (runtime.expParams);
		input->ao.resetDacEvents ();
		input->ttls.resetTtlEvents ();
		input->dds.resetDDSEvents();
		input->ol.resetOLEvents();

		input->ao.initializeDataObjects (0);
		input->ttls.initializeDataObjects (0);
		input->dds.initializeDataObjects(0);
		input->ol.initializeDataObjects(0);

		input->zynqExp.sendCommand("initExp");

		loadSkipTimes = std::vector<double> (variations);
		emit notification ("Analyzing Master Script...\n");
		std::string warnings;
		repeatMgrPtr = std::make_unique<repeatManager>(); //repeatManager repeatMgr;
		repeatManager& repeatMgr = *(repeatMgrPtr.get());
		analyzeMasterScript (input->ttls, input->ao,input->dds, input->ol, runtime.expParams, runtime.masterScript,
			runtime.mainOpts.atomSkipThreshold != UINT_MAX, warnings, operationTime, loadSkipTime, repeatMgr);
		
		emit notification("Calcualting Repeat Manager variations...\n", 1);
		repeatMgr.calculateVariations(runtime.expParams);
		emit notification ("Calcualting DO system variations...\n", 1);
		input->ttls.calculateVariations (runtime.expParams, this);
		emit notification ("Calcualting AO system variations...\n", 1);
		input->ao.calculateVariations (runtime.expParams, this, input->calibrations);
		emit notification("Calcualting DDS system variations...\n", 1);
		input->dds.calculateVariations(runtime.expParams, this, input->calibrations);
		emit notification("Calcualting OL system variations...\n", 1);
		input->ol.calculateVariations(runtime.expParams, this);

		emit notification("Constructing repeatitions for DO system...\n", 1);
		input->ttls.constructRepeats(repeatMgr);
		emit notification("Constructing repeatitions for AO system...\n", 1);
		input->ao.constructRepeats(repeatMgr);
		emit notification("Constructing repeatitions for DDS system...\n", 1);
		input->dds.constructRepeats(repeatMgr);
		emit notification("Constructing repeatitions for OL system...\n", 1);
		input->ol.constructRepeats(repeatMgr);
		
		emit notification ("Preparing DO, AO, DDS, OL for experiment and Running final ado checks...\n");
		for (auto variationInc : range (variations)) {
			if (isAborting) { thrower (abortString); }
			double& currLoadSkipTime = loadSkipTimes[variationInc];
			currLoadSkipTime = convertToTime (loadSkipTime, runtime.expParams, variationInc);
			input->aoSys.standardExperimentPrep (variationInc, runtime.expParams, currLoadSkipTime);
			input->ddsSys.standardExperimentPrep(variationInc);
			input->olSys.standardExperimentPrep(variationInc, input->ttls, warnings);
			input->ttlSys.standardExperimentPrep(variationInc, currLoadSkipTime, runtime.expParams);//make sure this is the last one, since offsetlock can insert in ttl sequence 
			
			input->ao.checkTimingsWork(variationInc);
		}
		emit warn(qstr(warnings));
		unsigned __int64 totalTime = 0;
		std::vector<double> finaltimes = input->ttls.getFinalTimes();
		double sum = std::accumulate(finaltimes.begin(), finaltimes.end(), 0.0);
		double mean = sum / finaltimes.size();
		emit notification (("Programmed average time per repetition: " + str (mean) + "\r\n").c_str (), 1);
		for (auto variationNumber : range (variations)) {
			totalTime += unsigned __int64 (finaltimes[variationNumber] * runtime.repetitions);
		}
		emit notification (("Programmed Total Experiment time: " + str (totalTime) + "\r\n").c_str (), 1);
		emit notification (("Number of TTL Events in experiment: " + str (input->ttls.getNumberEvents (0)) + "\r\n").c_str (), 1);
		emit notification (("Number of DAC Events in experiment: " + str (input->ao.getNumberEvents (0)) + "\r\n").c_str (), 1);
	}
}

void ExpThreadWorker::runConsistencyChecks (std::vector<parameterType> expParams, std::vector<calSettings> calibrations) {
	Sleep (1000);
	emit plot_Xvals_determined (ParameterSystem::getOrderedKeyValues (expParams));
	emit expParamsSet (expParams);
	//input->globalControl.setUsages (expParams);
	for (auto& var : expParams) {
		if (!var.constant && !var.active) {
			emit warn (cstr ("WARNING: Variable " + var.name + " is varied, but not being used?!?\r\n"));
		}
	}
	emit expCalibrationsSet(calibrations);

	checkTriggerNumbers(expParams);
}

void ExpThreadWorker::waitForSequenceFinish(double seqTime)
{
	const double minimumSleep = 10.0;
	const double maximumSleep = 300.0;
	const double targetSleep = seqTime * 0.1;
	double sleepTime = seqTime + minimumSleep;
	if (targetSleep > minimumSleep) {
		sleepTime = seqTime + targetSleep;
	}
	if (targetSleep > maximumSleep) {
		sleepTime = seqTime + maximumSleep;
	}
	Sleep(sleepTime);
}

void ExpThreadWorker::handlePause (std::atomic<bool>& isPaused, std::atomic<bool>& isAborting) {
	if (isAborting) { thrower (abortString); }
	if (isPaused) {
		emit notification ("PAUSED\r\n!");
		while (isPaused) {
			Sleep (100);
			if (isAborting) { thrower (abortString); }
		}
		emit notification ("UN-PAUSED!\r\n");
	}
}

void ExpThreadWorker::initVariation (unsigned variationInc,std::vector<parameterType> expParams) {
	auto variations = determineVariationNumber (expParams);
	emit notification (qstr("Variation #" + str (variationInc + 1) + "/" + str (variations) + ": \n"), 2);
	if (input->sleepTime != 0) { Sleep (input->sleepTime); }
	for (auto param : expParams) {
		if (param.valuesVary) {
			if (param.keyValues.size () == 0) {
				thrower ("Variable " + param.name + " varies, but has no values assigned to "
					"it! (This shouldn't happen, it's a low-level bug...)");
			}
			emit notification (qstr(param.name + ": " + str (param.keyValues[variationInc], 12) + "\r\n"), 2);
		}
	}
	//waitForAndorFinish ();
	bool skipOption = input->skipNext == nullptr ? false : input->skipNext->load ();
	if (true /*runMaster*/) { /*input->ttls.ftdi_write (variationInc, skipOption);*/ }
	handleDebugPlots(input->ttls, input->ao, input->ol, variationInc);
}

void ExpThreadWorker::waitForAndorFinish () {
	auto& andorCamera = input->devices.getSingleDevice<AndorCameraCore> ();
	while (true) {
		if (andorCamera.isRunning()/*andorCamera.queryStatus () == DRV_ACQUIRING*/) {
			Sleep (100);
			emit notification ("Waiting for Andor camera to finish acquisition...\n");
			if (isAborting) { thrower (abortString); }
		}
		else { break; }
	}
}

void ExpThreadWorker::errorFinish (std::atomic<bool>& isAborting, ChimeraError& exception,
	std::chrono::time_point<chronoClock> startTime) {
	//setExperimentGUIcolor();
	try {
		input->zynqExp.sendCommand("resetSeq");
		Sleep(50);
		input->zynqExp.sendCommand("resetSeq");
		Sleep(50);
		input->ttls.FPGAForceOutput(input->ttlSys.getCurrentStatus());
		Sleep(50);
		input->aoSys.setDACs();
		Sleep(50);
		input->ddsSys.setDDSs();
		Sleep(50);
		input->olSys.setOLs(input->ttls, input->ttlSys.getCurrentStatus());
		Sleep(50);
		input->ddsSys.relockPLL();
		Sleep(100);
		input->ttls.FPGAForceOutput(input->ttlSys.getCurrentStatus());
	}
	catch (ChimeraError& e) {
		emit warn("Failed to set default output for ZYNQ and/or Offsetlock after experiment ERROR-FINISH.\r\n" + e.qtrace(), 0);
	}


	std::string finMsg;
	if (isAborting) {
		emit updateBoxColor ("Grey", "Other");
		finMsg = abortString.c_str ();
	}
	else {
		emit updateBoxColor ("Red", "Other");
		finMsg = "Bad Exit!\r\nExited main experiment thread abnormally." + exception.trace ();
	}
	isAborting = false;
	auto exp_t = std::chrono::duration_cast<std::chrono::seconds>((chronoClock::now () - startTime)).count ();
	emit errorExperimentFinish ((finMsg + "\r\nExperiment took " + str (int (exp_t) / 3600) + " hours, "
		+ str (int (exp_t) % 3600 / 60) + " minutes, "+ str (int (exp_t) % 60) + " seconds.\r\n").c_str (), 
		input->profile);
}

void ExpThreadWorker::normalFinish (ExperimentType& expType, bool runMaster,
	std::chrono::time_point<chronoClock> startTime) {
	auto exp_t = std::chrono::duration_cast<std::chrono::seconds>((chronoClock::now () - startTime)).count ();
	try {
		setExperimentGUIcolor();
		input->zynqExp.sendCommand("resetSeq"); 
		Sleep(50);
		input->ttls.FPGAForceOutput(input->ttlSys.getCurrentStatus());
		Sleep(50);
		input->aoSys.setDACs();
		Sleep(50);
		input->ddsSys.setDDSs();
		Sleep(50);
		input->olSys.setOLs(input->ttls, input->ttlSys.getCurrentStatus());
		Sleep(50);
		input->ddsSys.relockPLL();
		Sleep(100);
		input->ttls.FPGAForceOutput(input->ttlSys.getCurrentStatus());
	}
	catch (ChimeraError& e) {
		emit warn("Failed to set default output for ZYNQ and/or Offsetlock after experiment NORMAL-FINISH.\r\nTHIS SHOULDN\'T HAPPEN\r\n" + e.qtrace(), 0);
	}
	switch (expType) {
	case ExperimentType::AutoCal:
		emit calibrationFinish (("\r\nCalibration Finished Normally.\r\nExperiment took "
			+ str (int (exp_t) / 3600) + " hours, " + str (int (exp_t) % 3600 / 60)
			+ " minutes, " + str (int (exp_t) % 60) + " seconds.\r\n").c_str (),
			input->profile);
		break;
	default:
		emit normalExperimentFinish (("\r\nExperiment Finished Normally.\r\nExperiment took "
			+ str (int (exp_t) / 3600) + " hours, " + str (int (exp_t) % 3600 / 60)
			+ " minutes, " + str (int (exp_t) % 60) + " seconds.\r\n").c_str (), input->profile);
	}
}

void ExpThreadWorker::startRep (unsigned repInc, unsigned variationInc, bool skip) {
	if (true /*runMaster*/) {
		//QTime timer;
		//timer.start();
		//emit notification (qstr ("Starting Repetition #" + qstr (repInc) + "\n"), 2);
		emit repUpdate (repInc + 1);
		input->zynqExp.sendCommand("initExp");
		Sleep(10);
		//input->aoSys.resetDacs (variationInc, skip);
		//input->ttls.ftdi_trigger ();
		//input->ttls.FtdiWaitTillFinished (variationInc);
		//input->aoSys.stopDacs();
		//input->aoSys.configureClocks(variationInc, skip);
		input->ao.writeDacs(variationInc, skip);
		input->dds.writeDDSs(variationInc, skip);
		input->ol.writeOLs(variationInc);
		input->ttls.writeTtlDataToFPGA(variationInc, skip);
		//emit notification("0.1: " + qstr(timer.elapsed()) + "\t");
		Sleep(50); /// have to sleep for this amount of time to make TCP connect smoothly?????? zzp 2021/06/04 very annoying
		input->zynqExp.sendCommand("trigger");
		//emit notification("0.2: " + qstr(timer.elapsed()) + "\n");
	}
}

void ExpThreadWorker::deviceLoadExpSettings (IDeviceCore& device, ConfigStream& cStream) {
	try {
		emit notification ("Loading Settings for device " + qstr (device.getDelim ()) + "...\n", 1);
		emit updateBoxColor ("White", device.getDelim ().c_str ());
		device.loadExpSettings (cStream);
	}
	catch (ChimeraError&) {
		emit updateBoxColor ("Red", device.getDelim ().c_str ());
		throwNested ("Error seen while loading experiment settings for system: " + device.getDelim ());
	}
}

void ExpThreadWorker::deviceProgramVariation (IDeviceCore& device, std::vector<parameterType>& expParams, 
	unsigned variationInc) {
	if (device.experimentActive) {
		try {
			emit notification (qstr ("Programming Devce " + device.getDelim () + "...\n"), 3);
			device.programVariation (variationInc, expParams, this);
			emit updateBoxColor ("Blue", device.getDelim ().c_str ());
		}
		catch (ChimeraError&) {
			emit updateBoxColor ("Red", device.getDelim ().c_str ());
			throwNested ("Error seen while programming variation for system: " + device.getDelim ());
		}
	}
}

void ExpThreadWorker::deviceCalculateVariations (IDeviceCore& device, std::vector<parameterType>& expParams) {
	if (device.experimentActive) {
		try {
			emit notification (qstr ("Calculating Variations for device " + device.getDelim ()
				+ "...\n"), 1);
			emit updateBoxColor ("Yellow", device.getDelim ().c_str ());
			device.calculateVariations (expParams, this);
		}
		catch (ChimeraError&) {
			emit updateBoxColor ("Red", device.getDelim ().c_str ());
			throwNested ("Error Seen while calculating variations for system: " + device.getDelim ());
		}
		if (isAborting) thrower (abortString);
	}
}

void ExpThreadWorker::deviceNormalFinish (IDeviceCore& device) {
	if (device.experimentActive) {
		emit updateBoxColor ("Green", device.getDelim ().c_str ());
		device.normalFinish ();
	}
	else {
		emit updateBoxColor ("Dark gray", device.getDelim ().c_str ());
	}
}

void ExpThreadWorker::calibrationOptionReport(const ExpRuntimeData& runtime)
{	
	// this should be placed after the first deviceCalculateVariations and calculateAdoVariations
	bool calUsed = false;
	for (auto& cal : input->calibrations) {
		calUsed = calUsed || cal.result.active;
	}
	if (!runtime.mainOpts.inExpCalibration && calUsed) {
		emit notification("Calibration is used in the experiment. In-Exp calibration is NOT enabled.\r\n", 0);
	}
	else if (runtime.mainOpts.inExpCalibration && !calUsed) {
		emit warn("Calibration is NOT used in the experiment but In-Exp calibration is enabled. Please make sure this is what you really want.\r\n", 0);
	}
	else if (!runtime.mainOpts.inExpCalibration && !calUsed) {
		emit notification("Calibration is NOT used in the experiment. In-Exp calibration is NOT enabled.\r\n", 0);
	}
	else if (runtime.mainOpts.inExpCalibration && calUsed) {
		emit notification("Calibration is used in the experiment. In-Exp calibration is enabled.\r\n", 0);
	}
}

void ExpThreadWorker::inExpCalibrationProcedure(ExpRuntimeData& runtime, bool calibrateOnlyExpActive)
{
	// this should be placed after the first deviceCalculateVariations and calculateAdoVariations
	if (!runtime.mainOpts.inExpCalibration) {
		return;
	}
	calibrateOnlyExpActive ?
		emit notification(qstr("In-Exp Calibration: Running Only Experiment Active Calibrations.\n"), 0) :
		emit notification(qstr("In-Exp Calibration: Running All Calibrations.\n"), 0);

	emit expCalibrationsSet(input->calibrations); // blocking connection, wait for slot to return so that alway set expCal before run another inexp cal.
	input->calManager->inExpRunAllThreaded(this, calibrateOnlyExpActive);
	std::unique_lock<std::mutex> lock(input->calManager->calLock());
	input->calManager->calConditionVariable().wait(lock, [this] {
		return !input->calManager->isCalibrationRunning(); });
	lock.unlock();
	emit notification("In-Exp Calibration: Finished calibration and proceed to recalculate variations", 1);

	input->calibrations = input->calManager->getCalibrationInfo();
	for (auto& cal : input->calibrations) {
		if (cal.active && !cal.calibrated && cal.result.active) {
			emit warn("In-Exp calibration failed for " + qstr(cal.result.calibrationName) + ". Falling back to the previous calibration result.\r\n", 0);
		}
		else if (cal.active && !cal.calibrated && !cal.result.active) {
			emit warn("In-Exp calibration failed for " + qstr(cal.result.calibrationName) + ". But this is not used in the experiment.\r\n", 1);
		}
	}

	auto variations = determineVariationNumber(runtime.expParams);
	repeatManager& repeatMgr = *(repeatMgrPtr.get());
	emit notification("Calcualting Repeat Manager variations...\n", 2);
	repeatMgr.calculateVariations(runtime.expParams);
	emit notification("In-Exp Calibration: Re-Calcualting DO system variations...\n", 2);
	input->ttls.calculateVariations(runtime.expParams, this);
	emit notification("In-Exp Calibration: Re-Calcualting AO system variations...\n", 2);
	input->ao.calculateVariations(runtime.expParams, this, input->calibrations);
	emit notification("In-Exp Calibration: Re-Calcualting DDS system variations...\n", 2);
	input->dds.calculateVariations(runtime.expParams, this, input->calibrations);
	emit notification("In-Exp Calibration: Re-Calcualting OL system variations...\n", 2);
	input->ol.calculateVariations(runtime.expParams, this);
	
	emit notification("Constructing repeatitions for DO system...\n", 2);
	input->ttls.constructRepeats(repeatMgr);
	emit notification("Constructing repeatitions for AO system...\n", 2);
	input->ao.constructRepeats(repeatMgr);
	emit notification("Constructing repeatitions for DDS system...\n", 2);
	input->dds.constructRepeats(repeatMgr);
	emit notification("Constructing repeatitions for OL system...\n", 2);
	input->ol.constructRepeats(repeatMgr);

	emit notification("In-Exp Calibration: Preparing DO, AO, DDS, OL for experiment and Running final ado checks...\n", 1);
	std::string warnings;
	for (auto variationInc : range(variations)) {
		double& currLoadSkipTime = loadSkipTimes[variationInc];
		currLoadSkipTime = convertToTime(loadSkipTime, runtime.expParams, variationInc);
		input->aoSys.standardExperimentPrep(variationInc, runtime.expParams, currLoadSkipTime);
		input->ddsSys.standardExperimentPrep(variationInc);
		input->olSys.standardExperimentPrep(variationInc, input->ttls, warnings);
		input->ttlSys.standardExperimentPrep(variationInc, currLoadSkipTime, runtime.expParams);//make sure this is the last one, since offsetlock can insert in ttl sequence 
		input->ao.checkTimingsWork(variationInc);
	}
	emit warn(qstr(warnings), 2);
	emit notification("In-Exp Calibration: Finished Re-Calcualting DAC/DDS system variations", 1);
}

void ExpThreadWorker::inExpCalibrationRun(ExpRuntimeData& runtime)
{
	if (input->calInterrupt->load()) {
		input->calInterrupt->store(false);
		inExpCalibrationProcedure(runtime, true);
	}
}

void ExpThreadWorker::loadExperimentRuntime (ConfigStream& config, ExpRuntimeData& runtime) {
	std::vector<parameterType> configParams = ParameterSystem::getConfigParamsFromFile(input->profile.configFilePath());
	runtime.expParams = ParameterSystem::combineParams(configParams, input->globalParameters);
	ScanRangeInfo varRangeInfo = ParameterSystem::getRangeInfoFromFile (input->profile.configFilePath ());
	loadMasterScript (ConfigSystem::getMasterAddressFromConfig (input->profile), runtime.masterScript);
	runtime.mainOpts = ConfigSystem::stdConfigGetter (config, "MAIN_OPTIONS", MainOptionsControl::getSettingsFromConfig);
	runtime.repetitions = ConfigSystem::stdConfigGetter (config, "REPETITIONS", Repetitions::getSettingsFromConfig);
	ParameterSystem::generateKey (runtime.expParams, runtime.mainOpts.randomizeVariations, varRangeInfo);
}

void ExpThreadWorker::setExperimentGUIcolor()
{
	emit input->ttlSys.setExperimentActiveColor(std::vector<DoCommand>(), true);
	emit input->aoSys.setExperimentActiveColor(std::vector<AoCommand>(), true);
	emit input->ddsSys.setExperimentActiveColor(std::vector<DdsCommand>(), true);
	emit input->olSys.setExperimentActiveColor(std::vector<OlCommand>(), true);

	try {
		emit input->ttlSys.setExperimentActiveColor(input->ttls.getTtlCommand(input->numVariations - 1), true);
		emit input->aoSys.setExperimentActiveColor(input->ao.getDacCommand(input->numVariations - 1), true);
		emit input->ddsSys.setExperimentActiveColor(input->dds.getDdsCommand(input->numVariations - 1), true);
		emit input->olSys.setExperimentActiveColor(input->ol.getOlCommand(input->numVariations - 1), true);
	}
	catch (ChimeraError& e) {
		emit warn("Error happens when resetting the GUI edit colors \n" + e.qtrace());
	}
	catch (...) {
		emit warn("Error happens when resetting the GUI edit colors \n");
	}

}
