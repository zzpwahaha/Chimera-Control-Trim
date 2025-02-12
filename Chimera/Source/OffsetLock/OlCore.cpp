#include "stdafx.h"
#include "OlCore.h"
#include <ExperimentMonitoringAndStatus/ExperimentSeqPlotter.h>
#include <qDebug>
#include <qelapsedtimer.h>

OlCore::OlCore(std::vector<bool> safemodes)
	//: qtFlumes{
	//QtSerialFlume(safemode, OL_COM_PORT[0]),
	//QtSerialFlume(safemode, OL_COM_PORT[1]) }
	: btFlumes{
	BoostAsyncSerial(safemodes[0], OL_COM_PORT[0], 9600, 8,
		boost::asio::serial_port_base::stop_bits::one,
		boost::asio::serial_port_base::parity::none,
		boost::asio::serial_port_base::flow_control::none),
	BoostAsyncSerial(safemodes[1], OL_COM_PORT[1], 9600, 8,
		boost::asio::serial_port_base::stop_bits::one,
		boost::asio::serial_port_base::parity::none,
		boost::asio::serial_port_base::flow_control::none),
	BoostAsyncSerial(safemodes[2], OL_COM_PORT[2], 115200, 8,
		boost::asio::serial_port_base::stop_bits::one,
		boost::asio::serial_port_base::parity::none,
		boost::asio::serial_port_base::flow_control::none) }
	, readComplete(true)
{
	for (auto& btFlume : btFlumes) {
		btFlume.setReadCallback(boost::bind(&OlCore::readCallback, this, _1));
		btFlume.setErrorCallback(boost::bind(&OlCore::errorCallback, this, _1));
	}
}

OlCore::~OlCore()
{
	//for (auto& qtFlume : qtFlumes) {
	//	qtFlume.close();
	//}
}


bool OlCore::isValidOLName(std::string name)
{
	for (unsigned olInc = 0; olInc < size_t(OLGrid::total); olInc++)
	{
		if (getOLIdentifier(name) != -1) {
			return true;
		}
	}
	return false;
}

int OlCore::getOLIdentifier(std::string name)
{
	for (unsigned olInc = 0; olInc < size_t(OLGrid::total); olInc++)
	{
		// check names set by user.
		std::transform(names[olInc].begin(), names[olInc].end(), names[olInc].begin(), ::tolower);
		if (name == names[olInc])
		{
			return olInc;
		}
		// check standard names which are always acceptable.
		if (name == "ol" +
			str(olInc / size_t(OLGrid::numPERunit)) + "_" +
			str(olInc % size_t(OLGrid::numPERunit)))
		{
			return olInc;
		}
	}
	// not an identifier.
	return -1;
}

void OlCore::setNames(std::array<std::string, size_t(OLGrid::total)> namesIn)
{
	names = std::move(namesIn);
}

std::string OlCore::getName(int olNumber)
{
	return names[olNumber];
}

std::array<std::string, size_t(OLGrid::total)> OlCore::getName()
{
	return names;
}

/**********************************************************************************************/



void OlCore::resetOLEvents()
{
	olCommandFormList.clear();
	olCommandList.clear();
	olSnapshots.clear();
	olChannelSnapshots.clear();
}

void OlCore::prepareForce()
{
	sizeDataStructures(1);
}

void OlCore::sizeDataStructures(unsigned variations)
{
	olCommandList.clear();
	olSnapshots.clear();
	olChannelSnapshots.clear();

	olCommandList.resize(variations);
	olSnapshots.resize(variations);
	olChannelSnapshots.resize(variations);
}

void OlCore::initializeDataObjects(unsigned variationNum)
{
	tmp = 0;
	olCommandFormList = std::vector<OlCommandForm>(variationNum);
	sizeDataStructures(variationNum);
}

std::vector<OlCommand> OlCore::getOlCommand(unsigned variation)
{
	if (olCommandList.size() - 1 < variation) {
		thrower("Aquiring OL command out of range");
	}
	return olCommandList[variation];
}

void OlCore::setOLCommandForm(OlCommandForm command)
{
	olCommandFormList.push_back(command);
	// you need to set up a corresponding trigger to tell the dacs to change the output at the correct time. 
	// This is done later on interpretation of ramps etc.
}

void OlCore::handleOLScriptCommand(OlCommandForm command, std::string name, std::vector<parameterType>& vars)
{
	if (command.commandName != "ol:" &&
		command.commandName != "ollinspace:" &&
		command.commandName != "olramp:")
	{
		thrower("ERROR: offsetlock commandName not recognized!");
	}
	if (!isValidOLName(name))
	{
		thrower("ERROR: the name " + name + " is not the name of a offsetlock!");
	}
	// convert name to corresponding dac line.
	command.line = getOLIdentifier(name);
	if (command.line == -1)
	{
		thrower("ERROR: the name " + name + " is not the name of a offsetlock!");
	}
	setOLCommandForm(command);
}


void OlCore::calculateVariations(std::vector<parameterType>& variables, ExpThreadWorker* threadworker)
{
	unsigned variations;
	variations = variables.front().keyValues.size();
	if (variations == 0)
	{
		variations = 1;
	}
	/// imporantly, this sizes the relevant structures.
	sizeDataStructures(variations);

	bool resolutionWarningPosted = false;
	bool nonIntegerWarningPosted = false;

	for (unsigned variationInc = 0; variationInc < variations; variationInc++)
	{
		for (unsigned eventInc = 0; eventInc < olCommandFormList.size(); eventInc++)
		{
			OlCommand tempEvent;
			tempEvent.line = olCommandFormList[eventInc].line;
			tempEvent.repeatId = olCommandFormList[eventInc].repeatId; // this set the repeatId for all steps below
			// Deal with time.
			if (olCommandFormList[eventInc].time.first.size() == 0)
			{
				// no variable portion of the time.
				tempEvent.time = olCommandFormList[eventInc].time.second;
			}
			else
			{
				double varTime = 0;
				for (auto variableTimeString : olCommandFormList[eventInc].time.first)
				{
					varTime += variableTimeString.evaluate(variables, variationInc);
				}
				tempEvent.time = varTime + olCommandFormList[eventInc].time.second;
			}

			if (olCommandFormList[eventInc].commandName == "ol:")
			{
				/// single point.
				////////////////
				// deal with amp
				tempEvent.value = olCommandFormList[eventInc].initVal.evaluate(variables, variationInc);
				tempEvent.endValue = tempEvent.value;
				tempEvent.numSteps = 1;
				tempEvent.rampTime = getTimeResolution(tempEvent.line);
				olCommandList[variationInc].push_back(tempEvent);
			}
			else if (olCommandFormList[eventInc].commandName == "ollinspace:")
			{
				// interpret ramp time command. I need to know whether it's ramping or not.
				double rampTime = olCommandFormList[eventInc].rampTime.evaluate(variables, variationInc);
				/// many points to be made.
				// convert initValue and finalValue to doubles to be used 
				double initValue, finalValue;
				int numSteps;
				initValue = olCommandFormList[eventInc].initVal.evaluate(variables, variationInc);
				// deal with final value;
				finalValue = olCommandFormList[eventInc].finalVal.evaluate(variables, variationInc);
				// deal with numPoints
				numSteps = olCommandFormList[eventInc].numSteps.evaluate(variables, variationInc);
				double rampInc = (finalValue - initValue) / double(numSteps);
				if ((fabs(rampInc) < olFreqResolution) && !resolutionWarningPosted)
				{
					resolutionWarningPosted = true;
					thrower("Warning: numPoints of " + str(numSteps) + " results in an amplitude ramp increment of "
						+ str(rampInc) + " is below the resolution of the offset lock (which is " + str(50) + "MHz/2^25 = "
						+ str(olFreqResolution) + "). \r\n");
				}
				double timeInc = rampTime / double(numSteps);
				double initTime = tempEvent.time;
				double currentTime = tempEvent.time;
				double val = initValue;

				for (auto stepNum : range(numSteps))
				{
					tempEvent.value = val;
					tempEvent.time = currentTime;
					tempEvent.endValue = val;
					tempEvent.rampTime = getTimeResolution(tempEvent.line);
					tempEvent.numSteps = 1;
					olCommandList[variationInc].push_back(tempEvent);
					currentTime += timeInc;
					val += rampInc;
				}
				 //and get the final amp.
				tempEvent.value = finalValue;
				tempEvent.time = initTime + rampTime;
				tempEvent.endValue = finalValue;
				tempEvent.rampTime = getTimeResolution(tempEvent.line);
				tempEvent.numSteps = 1;
				olCommandList[variationInc].push_back(tempEvent);
			}
			else if (olCommandFormList[eventInc].commandName == "olramp:")
			{
				double rampTime = olCommandFormList[eventInc].rampTime.evaluate(variables, variationInc);
				// convert initValue and finalValue to doubles to be used 
				double initValue, finalValue, numSteps;
				initValue = olCommandFormList[eventInc].initVal.evaluate(variables, variationInc);
				// deal with final value;
				finalValue = olCommandFormList[eventInc].finalVal.evaluate(variables, variationInc);
				// set votlage resolution to be maximum allowed by the ramp range and time
				numSteps = rampTime / getTimeResolution(tempEvent.line);
				double rampInc = (finalValue - initValue) / numSteps;
				if ((fabs(rampInc) < olFreqResolution) && !resolutionWarningPosted)
				{
					resolutionWarningPosted = true;
					thrower("Warning: numPoints of " + str(numSteps) + " results in a ramp increment of "
						+ str(rampInc) + " is below the frequency resolution of the offsetlock (which is 50/2^25 = "
						+ str(olFreqResolution) + "). Ramp will not run.\r\n");
				}
				if (numSteps > 0xffffffff) {
					thrower("Warning: numPoints of " + str(numSteps) +
						" is larger than the max num of steps of the offsetlock ramps. Ramp will not run. \r\n");
				}

				double initTime = tempEvent.time;

				// for olramp, pass the ramp points and time directly to a single olCommandList element
				tempEvent.value = initValue;
				tempEvent.endValue = finalValue;
				tempEvent.time = initTime;
				tempEvent.rampTime = rampTime;
				tempEvent.numSteps = unsigned int(numSteps);
				olCommandList[variationInc].push_back(tempEvent);
			}
			else
			{
				thrower("ERROR: Unrecognized offsetlock command name: " + olCommandFormList[eventInc].commandName);
			}
		}
	}
}

void OlCore::constructRepeats(repeatManager& repeatMgr)
{
	typedef OlCommand Command;
	/* this is to be done after ttlCommandList is filled with all variations. */
	if (olCommandFormList.size() == 0 || olCommandList.size() == 0) {
		thrower("No DAC Commands???");
	}

	unsigned variations = olCommandList.size();
	repeatMgr.saveCalculationResults(); // repeatAddedTime is changed during construction, need to save and reset it before and after the construction body
	auto* repearRoot = repeatMgr.getRepeatRoot();
	auto allDescendant = repearRoot->getAllDescendant();
	if (allDescendant.empty()) {
		return; // no repeats need to handle.
	}


	// iterate through all variations
	for (auto varInc : range(variations)) {
		auto& cmds = olCommandList[varInc];
		/*lines below should be all the same as the DoCore*/

		// Recursively add these repeat for always starting with maxDepth repeat. And also update the already constructed one to its parent layer
		// The loop will end when all commands is not associated with repeat, i.e. the maxDepth command's repeatId.repeatTreeMap is root
		while (true) {
			/*find the max depth repeated command*/
			auto maxDepthIter = std::max_element(cmds.begin(), cmds.end(), [&](const Command& a, const Command& b) {
				return (a.repeatId.repeatTreeMap.first < b.repeatId.repeatTreeMap.first); });
			Command maxDepth = *maxDepthIter;

			/*check if all command is with zero repeat. If so, exit the loop*/
			if (maxDepth.repeatId.repeatTreeMap == repeatInfoId::root) {
				break;
			}

			/*find the repeat num and the repeat added time with the unique identifier*/
			auto repeatIIter = std::find_if(allDescendant.begin(), allDescendant.end(), [&](TreeItem<repeatInfo>* a) {
				return (maxDepth.repeatId.repeatIdentifier == a->data().identifier); });
			if (repeatIIter == allDescendant.end()) {
				thrower("Can not find the ID for the repeat in the DoCommand with max depth of the tree. This is a low level bug.");
			}
			TreeItem<repeatInfo>* repeatI = *repeatIIter;
			unsigned repeatNum = repeatI->data().repeatNums[varInc];
			double repeatAddedTime = repeatI->data().repeatAddedTimes[varInc];
			/*find the parent of this repeat and record its repeatInfoId for updating the repeated ones*/
			TreeItem<repeatInfo>* repeatIParent = repeatI->parentItem();
			repeatInfoId repeatIdParent{ repeatIParent->data().identifier, repeatIParent->itemID() };
			/*collect command that need to be repeated*/
			std::vector<Command> cmdToRepeat;
			std::copy_if(cmds.begin(), cmds.end(), std::back_inserter(cmdToRepeat), [&](Command doc) {
				return (doc.repeatId.repeatIdentifier == maxDepth.repeatId.repeatIdentifier); });
			/*check if the repeated command is continuous in the cmds vector, it should be as the cmds is representing the script's order at this stage*/
			auto cmdToRepeatStart = std::search(cmds.begin(), cmds.end(), cmdToRepeat.begin(), cmdToRepeat.end(),
				[&](const Command& a, const Command& b) {
					return (a.repeatId.repeatIdentifier == b.repeatId.repeatIdentifier);
				});
			if (cmdToRepeatStart == cmds.end()) {
				thrower("The repeated command is not contiguous inside the CommandList, which is not suppose to happen.");
			}
			int cmdToRepeatStartPos = std::distance(cmds.begin(), cmdToRepeatStart);
			auto cmdToRepeatEnd = cmdToRepeatStart + cmdToRepeat.size(); // this will point to first cmd that is after those repeated one in CommandList
			/*if repeatNum is zero, delete the repeated command and reduce the time for those comand comes after the repeated commands and that is all*/
			if (repeatNum == 0) {
				cmds.erase(std::remove_if(cmds.begin(), cmds.end(), [&](Command doc) {
					return (doc.repeatId.repeatIdentifier == maxDepth.repeatId.repeatIdentifier); }), cmds.end());
				/*de-advance the time of thoses command that is later in CommandList than the repeat block*/
				cmdToRepeatEnd = cmds.begin() + cmdToRepeatStartPos; // this will point to first cmd that is after those repeated one in CommandList, since the repeated is removed, should equal to cmdToRepeatStart
				std::for_each(cmdToRepeatEnd, cmds.end(), [&](Command& doc) {
					doc.time -= repeatAddedTime; });
				continue;
			}
			/*transform the repeating commandlist to its parent repeatInfoId so that it can be repeated in its parents level*/
			// could also use this: std::transform(cmds.cbegin(), cmds.cend(), cmds.begin(), [&](Command doc) { with a return
			std::for_each(cmds.begin(), cmds.end(), [&](Command& doc) {
				if (doc.repeatId.repeatIdentifier == maxDepth.repeatId.repeatIdentifier) {
					if (doc.repeatId.placeholder) {
						doc.repeatId = repeatIdParent;
						doc.repeatId.placeholder = true; // doesn't really matter, since this will be deleted anyway.
					}
					else { doc.repeatId = repeatIdParent; }
				} });
			/*determine whether this cmdToRepeat is just a placeholder*/
			bool noPlaceholder = std::none_of(cmdToRepeat.begin(), cmdToRepeat.end(), [&](Command doc) {
				return doc.repeatId.placeholder; });
			int insertedCmdSize = 0;
			if (noPlaceholder) {
				/*start to insert the repeated 'cmdToRpeat' to end of the repeat block, after insertion, 'cmdToRepeatEnd' can not be used*/
				std::vector<Command> cmdToInsert;
				cmdToInsert.clear();
				for (unsigned repeatInc : range(repeatNum - 1)) {
					// if only repeat for once, below will be ignored, since the first repeat is already in the list
					/*transform the repeating commandlist to its parent repeatInfoId and also increment its time so that it can be repeated in its parents level*/
					std::for_each(cmdToRepeat.begin(), cmdToRepeat.end(), [&](Command& doc) {
						doc.repeatId = repeatIdParent;
						doc.time += repeatAddedTime; });
					cmdToInsert.insert(cmdToInsert.end(), cmdToRepeat.begin(), cmdToRepeat.end());
				}
				cmds.insert(cmdToRepeatEnd, cmdToInsert.begin(), cmdToInsert.end());
				insertedCmdSize = cmdToInsert.size();

			}
			else {
				/*with placeholder, check if this is the only placeholder, as it should be in most cases*/
				if (cmdToRepeat.size() > 1) {
					bool allPlaceholder = std::all_of(cmdToRepeat.begin(), cmdToRepeat.end(), [&](Command doc) {
						return doc.repeatId.placeholder; });
					if (allPlaceholder) {
						thrower("The command-to-repeat contains more than one placeholder. This shouldn't happen "
							"as Chimera will only insert one placeholder if there is no command in this repeat. "
							"This shouldn't come from nested repeat either, since Chimera will add placeholder for each level of repeats "
							"if the there is no command. And the inferior level repeat placeholder should already be deleted after previous loop"
							"A low level bug.");
					}
					else {
						thrower("The command-to-repeat contains placeholder but also other commands that is not meant for placeholding. "
							"This shouldn't happen as Chimera will only insert placeholder if there is no command in this repeat. "
							"This shouldn't come from nested repeat either, inferior level repeat placeholder should already be deleted after previous loop. "
							"A low level bug.");
					}
				}
				else {
					/*remove the placeholder for this level of repeat. Other level would have their own placeholder inserted already if needed.*/
					cmds.erase(cmds.begin() + cmdToRepeatStartPos);
					insertedCmdSize = -1; // same as '-cmdToRepeat.size()'
				}
			}
			/*advance the time of thoses command that is later in CommandList than the repeat block*/
			cmdToRepeatEnd = cmds.begin() + cmdToRepeatStartPos + cmdToRepeat.size() + insertedCmdSize;
			std::for_each(cmdToRepeatEnd, cmds.end(), [&](Command& doc) {
				doc.time += repeatAddedTime * (repeatNum - 1); });
			/*advance the time of the parent repeat, if the parent is not root*/
			if (repeatIParent != repearRoot) {
				repeatIParent->data().repeatAddedTimes[varInc] += repeatAddedTime * repeatNum;
			}
		}
	}
	repeatMgr.loadCalculationResults();
}

bool OlCore::repeatsExistInCommandForm(repeatInfoId repeatId)
{
	typedef OlCommandForm CommandForm;
	auto& cmdFormList = olCommandFormList;
	auto cmdFormRepeated = std::find_if(cmdFormList.begin(), cmdFormList.end(),
		[&](const CommandForm& a) {
			return (a.repeatId.repeatIdentifier == repeatId.repeatIdentifier);
		});
	return (cmdFormRepeated != cmdFormList.end());
}

void OlCore::addPlaceholderRepeatCommand(repeatInfoId repeatId)
{
	typedef OlCommandForm CommandForm;
	auto& cmdFormList = olCommandFormList;
	repeatId.placeholder = true;
	CommandForm cmdForm;
	cmdForm.commandName = "ol:";
	cmdForm.initVal = Expression("0.0");
	cmdForm.repeatId = repeatId;
	cmdFormList.push_back(cmdForm); // should be an alert for all other commands, and it will be cleansed after advancing the time
}


void OlCore::organizeOLCommands(unsigned variation, OlSnapshot initSnap, std::string& warning)
{
	// timeOrganizer: each element of this is a different time (the double), and associated with each time is a vector which locates 
	// which commands were at this time, for ease of retrieving all of the values in a moment.
	
	std::vector<OlCommand> tempEvents(olCommandList[variation]);
	// sort the events by time. using a lambda here.
	std::sort(tempEvents.begin(), tempEvents.end(),
		[](OlCommand a, OlCommand b) {return a.time < b.time; });
	//timeOrganizer.clear();
	//timeOrganizer.push_back({ ZYNQ_DEADTIME, std::vector<OlCommand>{} });
	//for (unsigned short i = 0; i < size_t(OLGrid::total); i++) {
	//	timeOrganizer.back().second.push_back( OlCommand{ i,ZYNQ_DEADTIME,status[i],status[i],0,-1 } );
	//}


	for (unsigned short channel = 0; channel < size_t(OLGrid::total); channel++)
	{
		auto& timeOgzer = timeOrganizer[channel];
		timeOgzer.clear();
		// intentionally avoid colliding with DO at t = ZYNQ_DEADTIME since DO will assert state status at that time
		timeOgzer.push_back({ ZYNQ_DEADTIME * 2,
			OlCommand{channel,ZYNQ_DEADTIME * 2,initSnap.olValues[channel],initSnap.olValues[channel],1,getTimeResolution(channel) } });

		for (unsigned commandInc = 0; commandInc < tempEvents.size(); commandInc++)
		{
			if (tempEvents[commandInc].line != channel) { continue; }
			// for each channel with a freq gap, bridge it by adding command at the end of last command
			const auto& tE = tempEvents[commandInc];
			if (timeOgzer.empty()) {
				timeOgzer.push_back({ tE.time, tE });
			}
			else {
				/// 2022/08/23 ZZP, seems like do not need this artificial bridge
				//if (fabs(timeOgzer.back().second.endValue - tE.value) > 2 * olFreqResolution)
				//{
				//	warning += "Warning from OffsetLock: output(" + OlCore::getName(tE.line)
				//		+ "), the frequency between two consecutive event point t=" + str(timeOgzer.back().first, 3)
				//		+ ", endfreq=" + str(timeOgzer.back().second.endValue, 3) + " and t=" + str(tE.time, 6) + ", startfreq="
				//		+ str(tE.value, 3) + " are not the same. Have bridged it with a command inserted at t="
				//		+ str(tE.time + OL_TIME_RESOLUTION, 3) + "\r\n";
				//	timeOgzer.push_back({ tE.time,
				//		OlCommand{channel,tE.time,timeOgzer.back().second.endValue,tE.value,1,OL_TIME_RESOLUTION} });
				//}
				// because the events are sorted by time, the time organizer will already be sorted by time, and therefore I 
				// just need to check the back value's time.
				double timediff = (tE.time - timeOgzer.back().first); //had better be positive, otherwise indicating a bridge is inserted perviously
				if (timediff >= getTimeResolution(channel) - 2 * DBL_EPSILON)
				{
					timeOgzer.push_back({ tE.time, tE });
				}
				else
				{
					if (timediff < -getTimeResolution(channel) - 2 * DBL_EPSILON) {
						thrower("Error in timeOrganizer, the time seems not sorted. A low level bug.");
					}
					// time that cannot be resolved by offset lock, complain it if it is of the same line
					warning += "Warning from OffsetLock: output(" + OlCore::getName(tE.line)
						+ "), the time between two consecutive event point t=" + str(timeOgzer.back().first, 3) + " and t="
						+ str(tE.time, 3) + " is below the offset lock resolution: " + str(getTimeResolution(channel), 3)
						+ "ms. Have make the later time exact " + str(3 * getTimeResolution(channel), 3) + "ms away\r\n";
					timeOgzer.push_back({ timeOgzer.back().first + 3 * getTimeResolution(channel), tE });
				}
			}

		}
	}
	//for (unsigned commandInc = 0; commandInc < tempEvents.size(); commandInc++)
	//{
	//	// because the events are sorted by time, the time organizer will already be sorted by time, and therefore I 
	//	// just need to check the back value's time.
	//	double timediff = fabs(tempEvents[commandInc].time - timeOrganizer.back().first);
	//	if (commandInc == 0 || timediff >= OL_TIME_RESOLUTION/*2 * DBL_EPSILON*/)
	//	{
	//		timeOrganizer.push_back({ tempEvents[commandInc].time,
	//								std::vector<OlCommand>({ tempEvents[commandInc] }) });
	//	}
	//	else if (timediff < OL_TIME_RESOLUTION)
	//	{
	//		// time that cannot be resolved by offset lock, complain it if it is of the same line
	//		bool exist = false;
	//		for (auto& cmd : timeOrganizer.back().second) {
	//			if (tempEvents[commandInc].line == cmd.line) {
	//				exist = true;
	//			}
	//		}
	//		if (exist) {
	//			timeOrganizer.push_back({ timeOrganizer.back().first + OL_TIME_RESOLUTION,
	//								std::vector<OlCommand>({ tempEvents[commandInc] }) });
	//			warning += "Warning from OffsetLock: output(" + OlCore::getName(tempEvents[commandInc].line)
	//				+ "), the time between two consecutive event point t=" + str(timeOrganizer.back().first, 3) + "and t="
	//				+ str(tempEvents[commandInc].time, 3) + " is below the offset lock resolution: " + str(OL_TIME_RESOLUTION, 3) 
	//				+ "ms. Have make the later time exact " + str(OL_TIME_RESOLUTION, 3) + "away";
	//		}
	//		else {
	//			// really is a new time for that line
	//			if (timediff < 2 * DBL_EPSILON) {
	//				// old time
	//				timeOrganizer.back().second.push_back(tempEvents[commandInc]);
	//			}
	//			else {
	//				//new time
	//				timeOrganizer.push_back({ tempEvents[commandInc].time,
	//								std::vector<OlCommand>({ tempEvents[commandInc] }) });
	//			}
	//		}
	//	}
	//}

	///// make the snapshots
	///// The following code is wrong, if want snapshot, need to combine all channels
	///// together, say ch1 need 3 changes, and ch2 need 1 change, unless ch2 change at the same time as
	///// ch1, there would be 4 snapshots. ZZP 06/27/2021
	//auto& snap = olSnapshots[variation];
	//snap.clear();
	//for (unsigned short channel = 0; channel < size_t(OLGrid::total); channel++)
	//{
	//	auto& timeOgzer = timeOrganizer[channel];
	//	if (timeOgzer.size() == 0) {
	//		continue; // no commands, that's fine.
	//	}
	//	snap.push_back(initSnap);
	//	if (timeOgzer[0].first != 0) {
	//		// then there were no commands at time 0, so just set the initial state to be exactly the original state before
	//		// the experiment started. I don't need to modify the first snapshot in this case, it's already set. Add a snapshot
	//		// here so that the thing modified is the second snapshot not the first. 
	//		snap.push_back(initSnap);
	//	}
	//	unsigned cnts = 0;
	//	for (auto& command : timeOgzer) {
	//		if (cnts != 0) {
	//			// handle the zero case specially. This may or may not be the literal first snapshot.
	//			// first copy the last set so that things that weren't changed remain unchanged.
	//			snap.push_back(snap.back());
	//		}
	//		snap.back().time = command.first;
	//		auto& change = command.second;
	//		// see description of this command above... update everything that changed at this time.
	//		snap.back().olValues[change.line] = change.value;
	//		snap.back().olEndValues[change.line] = change.endValue;
	//		snap.back().olRampTimes[change.line] = change.rampTime;
	//		cnts++;
	//	}
	//}
	
}

void OlCore::makeFinalDataFormat(unsigned variation, DoCore& doCore)
{
	OlChannelSnapshot channelSnapshot;
	//for each channel with a changed freq, bridge it by adding command at the end of last command
	for (unsigned short channel = 0; channel < size_t(OLGrid::total); channel++)
	{
		auto& timeOgzer = timeOrganizer[channel];
		for (unsigned commandInc = 0; commandInc < timeOgzer.size(); commandInc++)
		{
			auto& to = timeOgzer[commandInc];
			channelSnapshot.val = to.second.value;
			channelSnapshot.endVal = to.second.endValue;
			channelSnapshot.rampTime = to.second.rampTime;
			channelSnapshot.numSteps = to.second.numSteps;

			channelSnapshot.time = to.first;
			channelSnapshot.channel = to.second.line;
			if (commandInc != 0) {
				// only respond to command that make a difference in freq, other command could be there simply for 'repeat' placeholder or re-instating
				const auto& channelSnapPre = olChannelSnapshots[variation].back();
				if (channelSnapPre.channel != channelSnapshot.channel) {
					thrower("Channel number in Offsetlock commands are conflicting: should be Channel " + str(channelSnapPre.channel) + ", but sees Channel"
						+ str(channelSnapshot.channel) + " instead in variation " + str(variation) + ". This is a low level bug.");
				}
				if ((fabs(channelSnapshot.val - channelSnapshot.endVal) < olFreqResolution) &&
					(fabs(channelSnapPre.val - channelSnapPre.endVal) < olFreqResolution)) { // this one and previous one is not ramping, value=endValue
					if (fabs(channelSnapshot.val - channelSnapPre.endVal) < olFreqResolution) { // this one's value is the same as previous end value, should ignore
						continue;
					}

				}
			}
			olChannelSnapshots[variation].push_back(channelSnapshot);
			/*within timeOrganizer[i], the time are the same*/
			doCore.ttlPulseDirect(OL_TRIGGER_LINE[channel].first, OL_TRIGGER_LINE[channel].second, 
				channelSnapshot.time, OL_TRIGGER_TIME, variation);
		}
		if (olChannelSnapshots[variation].size() > maxCommandNum) {
			thrower("Ol command number " + str(olChannelSnapshots[variation].size()) + " is greater than the maximum Ol command that the"
				"microcontroller can accept, which is " + str(maxCommandNum));
		}
	}

}

std::vector<std::vector<plotDataVec>> OlCore::getPlotData(unsigned var)
{
	unsigned linesPerPlot = size_t(OLGrid::total) / ExperimentSeqPlotter::NUM_OL_PLTS;
	std::vector<std::vector<plotDataVec>> olData(ExperimentSeqPlotter::NUM_OL_PLTS,
		std::vector<plotDataVec>(linesPerPlot));
	if (olChannelSnapshots.size() <= var) {
		thrower("Attempted to use offsetlock data from variation " + str(var) + ", which does not exist!");
	}
	for (auto olchSnapn : range(olChannelSnapshots[var].size())) {
		auto& olchSnap = olChannelSnapshots[var][olchSnapn];
		auto& data = olData[olchSnap.channel / linesPerPlot][olchSnap.channel % linesPerPlot];
		if (olchSnapn != 0) {
			data.push_back({ olchSnap.time, olChannelSnapshots[var][olchSnapn - 1].val, 0 });
		}
		data.push_back({ olchSnap.time, olChannelSnapshots[var][olchSnapn].val, 0 });
	}
	return olData;
}

//void OlCore::standardExperimentPrep(unsigned variation, DoCore& doCore, std::string& warning)
//{
//	organizeOLCommands(variation, warning);
//	makeFinalDataFormat(variation, doCore);
//}


void OlCore::writeOLs(unsigned variation)
{
	unsigned short flumesIdx = 0;
	for (auto& btFlume : btFlumes/*auto& qtFlume : qtFlumes*/) {
		//qtFlume.getPort().clear();
		std::string buffCmd;
		for (auto& channelSnap : olChannelSnapshots[variation])
		{
			if (getFlumeIdx( channelSnap.channel ) == flumesIdx) {
				buffCmd += "(" + str(getCmdChannelIdx(channelSnap.channel)) + ","
					+ str(channelSnap.val, numFreqDigits) + ","
					+ str(channelSnap.endVal, numFreqDigits) + "," + str(channelSnap.numSteps) + ","
					+ str(channelSnap.rampTime, numTimeDigits(channelSnap.channel)) + ")";
			}
		}
		buffCmd += "e";

		int current_fails = 0;
label:
		QElapsedTimer timer;
		timer.start();
		std::string recv;
		readRegister.clear();
		errorMsg.clear();
		readComplete = false;
		std::vector<unsigned char> byteBuffCmd(buffCmd.cbegin(), buffCmd.cend());
		if (buffCmd != "e") {
			//recv = qtFlume.query(buffCmd);
			btFlume.write(byteBuffCmd);
			// want to try rethrow the possible exception with Chimera, but it is still not handled in expThread. ???????
			if (auto e = btFlume.lastException()) {
				try {
					boost::rethrow_exception(e);
				}
				catch (boost::system::system_error& e) {
					throwNested("Error seen in writing to serial port " + str(btFlume.portID) + ". Error: " + e.what());
				}
			}
		}
		else {
			recv = "Nothing programmed in OffsetLock in variation" + str(variation);
			continue;
		}
		qDebug() << tmp << qstr(buffCmd) << "Total time:" << timer.elapsed() << "ms";
		if (btFlume.safemode) {
			qDebug() << "I am in safemode: " << qstr(btFlume.portID);
			continue;
		}
		for (auto idx : range(200)) {
			if (readComplete) {
				recv = std::string(readRegister.cbegin(), readRegister.cend());
				break;
			}
			Sleep(5);
		}
		qDebug() << ">> After read:" << timer.elapsed() << "ms";
		
		if (recv.empty() || (!errorMsg.empty())) {
			// this could just be the case where we missed the return or the write is flaky, try to re-write
			if (current_fails < 3) {
				current_fails++;
				qDebug() << current_fails << "OFFSETLOCK Read nothing but tried to re-write to OFFSETLOCK!";
				goto label;
			}
			thrower("Nothing feeded back from Teensy after writing, something might be wrong with it." + recv + "\r\nError message: " + errorMsg);
			qDebug() << tmp << qstr("Empty return");
		}
		else {
			qDebug() << tmp << qstr(recv);
			std::transform(recv.begin(), recv.end(), recv.begin(), ::tolower); /*:: without namespace select from global namespce, see https://stackoverflow.com/questions/5539249/why-cant-transforms-begin-s-end-s-begin-tolower-be-complied-successfu*/
			if (recv.find("error") != std::string::npos) {
				thrower("Error in offset lock programming, from Teensy: " + recv + "\r\nNote each number can only be of 13 chars long");
			}
		}
		tmp++;
		flumesIdx++;
	}
}

void OlCore::OLForceOutput(std::array<double,size_t(OLGrid::total)> status, DoCore& doCore, DOStatus dostatus)
{
	resetOLEvents();
	initializeDataObjects(1);
	for (unsigned short inc = 0; inc < size_t(OLGrid::total); inc++)
	{
		olChannelSnapshots[0].push_back({ inc,0.1,status[inc],status[inc],1,1 });
	}
	writeOLs(0);
	doCore.FPGAForcePulse(dostatus, OL_TRIGGER_LINE, OL_TRIGGER_TIME);
	
}

void OlCore::resetConnection()
{
	//for (auto& qtFlume : qtFlumes) {
	//	qtFlume.resetConnection();
	//}
	for (auto& btFlume : btFlumes) {
		btFlume.disconnect();
		Sleep(10);
		btFlume.reconnect();
	}
}

void OlCore::readCallback(int byte)
{
	if (byte < 0 || byte >255) {
		thrower("Byte value readed needs to be in range 0-255.");
	}
	readRegister.push_back(byte);
	if (byte == '\n') {
		readComplete = true;
	}
}

void OlCore::errorCallback(std::string error)
{
	errorMsg = error;
}

unsigned short OlCore::getNumCmdChannel(unsigned short flumeIdx)
{
	auto totalFlume = static_cast<unsigned short>(OLGrid::numOFunit);
	if (flumeIdx < 0 || flumeIdx >= totalFlume) {
		thrower("Flume number: " + str(flumeIdx) + ", in offsetlock is out of the total flume number: " + str(totalFlume));
	}
	unsigned short channel = -1;
	switch (flumeIdx) {
	case 0:
	case 1:
		channel = 2;
		break;
	case 2:
		channel = 1;
		break;
	default:
		break;
	}
	return channel;
}

unsigned short OlCore::getFlumeIdx(unsigned short channel)
{
	auto totalChannel = static_cast<unsigned short>(OLGrid::total);
	if (channel < 0 || channel >= totalChannel) {
		thrower("Channel number: " + str(channel) + ", in offsetlock is out of the total channel number: " + str(totalChannel));
	}
	unsigned short flumeIdx = -1;
	switch (channel) {
	case 0:
	case 1:
		flumeIdx = 0;
		break;
	case 2:
	case 3:
		flumeIdx = 1;
		break;
	case 4:
		flumeIdx = 2;
		break;
	default:
		break;
	}
	return flumeIdx;

};

unsigned short OlCore::getCmdChannelIdx(unsigned short channel)
{
	auto flumeIdx = getFlumeIdx(channel);
	unsigned short cmdChannelIdx = -1;
	switch (flumeIdx) {
	case 0:
	case 1:
		cmdChannelIdx = channel % 2;
		break;
	case 2:
		cmdChannelIdx = 0;
		break;
	default:
		break;
	}
	return cmdChannelIdx;
};

double OlCore::getTimeResolution(unsigned short channel)
{
	auto flumeIdx = getFlumeIdx(channel);
	double timeResolution = -1.0;
	switch (flumeIdx) {
	case 0:
	case 1:
		timeResolution = OL_TIME_RESOLUTION;
		break;
	case 2:
		timeResolution = 0.2;
		break;
	default:
		break;
	}
	return timeResolution;
}

int OlCore::numTimeDigits(unsigned short channel)
{
	return static_cast<int>(abs(round(log10(getTimeResolution(channel)) - 0.49)));
}
