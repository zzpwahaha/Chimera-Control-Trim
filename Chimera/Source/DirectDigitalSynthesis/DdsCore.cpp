﻿#include "stdafx.h"
#include "ConfigurationSystems/ConfigSystem.h"
#include "DdsCore.h"

#include "ZynqTcp/ZynqTcp.h" /*only for the DDS_TIME_RESOLUTION macro*/

DdsCore::DdsCore ( bool safemode ) 
	: ftFlume ( safemode )
{
	//connectasync ( );
	//lockPLLs ( );
}

DdsCore::~DdsCore ( ){
	disconnect ( );
}

void DdsCore::assertDdsValuesValid ( std::vector<parameterType>& params ){
	unsigned variations = ( ( params.size ( ) ) == 0 ) ? 1 : params.front ( ).keyValues.size ( );
	for (auto& ramp : expRampList)	{
		ramp.rampTime.assertValid (params, GLOBAL_PARAMETER_SCOPE);
		ramp.freq1.assertValid (params, GLOBAL_PARAMETER_SCOPE);
		ramp.freq2.assertValid (params, GLOBAL_PARAMETER_SCOPE);
		ramp.amp1.assertValid (params, GLOBAL_PARAMETER_SCOPE);
		ramp.amp2.assertValid (params, GLOBAL_PARAMETER_SCOPE);
		ramp.rampTime.assertValid (params, GLOBAL_PARAMETER_SCOPE);
	}
}
/*called from ExpThreadWorker::deviceCalculateVariations, ddscore is to be removed from device*/
void DdsCore::calculateVariations (std::vector<parameterType>& params, ExpThreadWorker* threadworker){
	//evaluateDdsInfo (params);
	//unsigned variations = ((params.size ()) == 0) ? 1 : params.front ().keyValues.size ();
	//generateFullExpInfo (variations);
}

// this probably needs an overload with a default value for the empty parameters case...
void DdsCore::evaluateDdsInfo ( std::vector<parameterType> params ){
	unsigned variations = ( ( params.size ( ) ) == 0 ) ? 1 : params.front ( ).keyValues.size ( );
	for ( auto variationNumber : range ( variations ) )	{
		for ( auto& ramp : expRampList)	{
			ramp.rampTime.internalEvaluate (params, variations );
			ramp.freq1.internalEvaluate (params, variations );
			ramp.freq2.internalEvaluate (params, variations );
			ramp.amp1.internalEvaluate (params, variations );
			ramp.amp2.internalEvaluate (params, variations );
			ramp.rampTime.internalEvaluate (params, variations );
		}
	}
}

void DdsCore::programVariation ( unsigned variationNum, std::vector<parameterType>& params, 
								 ExpThreadWorker* threadworker){
	clearDdsRampMemory ( );
	auto& thisExpFullRampList = fullExpInfo ( variationNum );
	if ( thisExpFullRampList.size ( ) == 0 ){
		return;
	}
	lockPLLs ( );
	writeAmpMultiplier ( 0 );
	writeAmpMultiplier ( 1 );
	for ( auto boardNum : range ( thisExpFullRampList.front ( ).rampParams.numBoards ( ) ) ){
		for ( auto channelNum : range ( thisExpFullRampList.front ( ).rampParams.numChannels ( ) ) ){
			writeResetFreq ( boardNum, channelNum, thisExpFullRampList.front ( ).rampParams ( boardNum, channelNum ).freq1 );
			writeResetAmp ( boardNum, channelNum, thisExpFullRampList.front ( ).rampParams ( boardNum, channelNum ).amp1 );
		}
	}
	for ( auto rampIndex : range ( thisExpFullRampList.size ( ) ) ){
		writeOneRamp ( thisExpFullRampList[ rampIndex ], rampIndex );
	}
	longUpdate ( );
}


void DdsCore::writeOneRamp ( ddsRampFinFullInfo boxRamp, UINT8 rampIndex ){
	UINT16 reps = getRepsFromTime ( boxRamp.rampTime );
	writeRampReps ( rampIndex, reps );
	for ( auto boardNum : range ( boxRamp.rampParams.numBoards ( ) ) ){
		for ( auto channelNum : range ( boxRamp.rampParams.numChannels ( ) ) ){
			auto& channel = boxRamp.rampParams ( boardNum, channelNum );
			writeRampDeltaFreq ( boardNum, channelNum, rampIndex, ( channel.freq2 - channel.freq1 ) / reps );
			writeRampDeltaAmp ( boardNum, channelNum, rampIndex, ( channel.amp2 - channel.amp1 ) / reps );
		}
	}
}

void DdsCore::generateFullExpInfo (unsigned numVariations){
	fullExpInfo.resizeVariations (numVariations);
	for (auto varInc : range (numVariations)){
		fullExpInfo (varInc) = analyzeRampList (expRampList, varInc);
	}
}

/*
converts the list of individual set ramps, as set by the user, to the full ramp list which contains the state
of each dds at each point in the ramp.
*/
std::vector<ddsRampFinFullInfo> DdsCore::analyzeRampList ( std::vector<ddsIndvRampListInfo> rampList, unsigned variation ){
	// always rewrite the full vector
	unsigned maxIndex = 0;
	for ( auto& rampL : rampList ){
		maxIndex = ( ( maxIndex > rampL.index ) ? maxIndex : rampL.index );
	}
	std::vector<ddsRampFinFullInfo> fullRampInfo ( maxIndex + 1 );

	for ( auto& rampL : rampList ){
		auto& rampF = fullRampInfo[ rampL.index ].rampParams ( rampL.channel / 4, rampL.channel % 4 );
		if ( rampF.explicitlySet == true ){
			if ( fullRampInfo[ rampL.index ].rampTime != rampL.rampTime.getValue ( variation ) ){
				thrower ( "The ramp times of different channels on the same ramp index must match!" );
			}
		}
		else{
			fullRampInfo[ rampL.index ].rampTime = rampL.rampTime.getValue ( variation );
			rampF.explicitlySet = true;
		}
		rampF.freq1 = rampL.freq1.getValue ( variation );
		rampF.freq2 = rampL.freq2.getValue ( variation );
		rampF.amp1 = rampL.amp1.getValue ( variation );
		rampF.amp2 = rampL.amp2.getValue ( variation );
	}
	return fullRampInfo;
}


void DdsCore::forceRampsConsistent ( ){
	for ( auto varInc : range ( fullExpInfo.getNumVariations ( ) ) ){
		if ( fullExpInfo ( varInc ).size ( ) == 0 ) continue;
		if ( fullExpInfo ( varInc ).size ( ) == 1 ) continue; // no consistency to check if only 1. 
		auto& thisExpRampList = fullExpInfo ( varInc );
		for ( auto boardInc : range ( thisExpRampList.front ( ).rampParams.numBoards ( ) ) ){
			for ( auto channelInc : range ( thisExpRampList.front ( ).rampParams.numChannels ( ) ) ){
				// find the initial values
				double currentDefaultFreq, currentDefaultAmp;
				bool used = false;
				for ( auto rampInc : range ( thisExpRampList.size ( ) ) ){
					if ( thisExpRampList[ rampInc ].rampParams ( boardInc, channelInc ).explicitlySet ){
						currentDefaultFreq = thisExpRampList[ rampInc ].rampParams ( boardInc, channelInc ).freq1;
						currentDefaultAmp = thisExpRampList[ rampInc ].rampParams ( boardInc, channelInc ).amp1;
						used = true;
						break;
					}
				}
				if ( !used ){
					currentDefaultFreq = defaultFreq;
					currentDefaultAmp = defaultAmp;
				}
				for ( auto rampInc : range ( thisExpRampList.size ( ) - 1 ) ){
					auto& thisRamp = thisExpRampList[ rampInc ].rampParams ( boardInc, channelInc );
					auto& nextRamp = thisExpRampList[ rampInc + 1 ].rampParams ( boardInc, channelInc );
					bool bothExplicit = ( thisRamp.explicitlySet && nextRamp.explicitlySet );

					// this first if clause is basically just to handle the special case of the first ramp.
					if ( thisRamp.explicitlySet == false ){
						thisRamp.freq1 = thisRamp.freq2 = currentDefaultFreq;
						thisRamp.amp1 = thisRamp.amp2 = currentDefaultAmp;
						thisRamp.explicitlySet = true;
					}
					currentDefaultFreq = thisRamp.freq2;
					currentDefaultAmp = thisRamp.amp2;

					if ( nextRamp.explicitlySet == false ){
						thisRamp.freq1 = thisRamp.freq2 = currentDefaultFreq;
						thisRamp.amp1 = thisRamp.amp2 = currentDefaultAmp;
						nextRamp.explicitlySet = true;
					}
					// then a new ramp that the user set explicitly has been reached, and I need to check that
					// the parameters match up.
					if ( thisRamp.freq2 != nextRamp.freq1 || thisRamp.amp2 != nextRamp.amp1 ){
						thrower ( "Incompatible explicitly set ramps!" );
					}
				}
			}
		}
	}
}

// a wrapper around the ftFlume open
void DdsCore::connectasync ( ){
	DWORD numDevs = ftFlume.getNumDevices ( );
	if ( numDevs > 0 ){
		// hard coded for now but you can get the connected serial numbers via the getDeviceInfoList function.
		ftFlume.open ( "FT1FJ8PEB" );
		ftFlume.setUsbParams ( );
		connType = ddsConnectionType::type::Async;
	}
	else{
		thrower ( "No devices found." );
	}
}

void DdsCore::disconnect ( ){
	ftFlume.close ( );
}

std::string DdsCore::getSystemInfo ( ){
	unsigned numDev;
	std::string msg = "";
	try{
		numDev = ftFlume.getNumDevices ( );
		msg += "Number ft devices: " + str ( numDev ) + "\n";
	}
	catch ( ChimeraError& err ){
		msg += "Failed to Get number ft Devices! Error was: " + err.trace ( );
	}
	msg += ftFlume.getDeviceInfoList ( );
	return msg;
}

/* Get Frequency Tuning Word - convert a frequency in double to  */
INT DdsCore::getFTW ( double freq ){
	// Negative ints, Nyquist resetFreq, works out.
	if ( freq > INTERNAL_CLOCK / 2 ){
		thrower ( "DDS frequency out of range. Must be < 250MHz." );
		return 0;
	}
	return (INT) round ( ( freq * pow ( 2, 32 ) ) / ( INTERNAL_CLOCK ) );;
}

unsigned DdsCore::getATW ( double amp ){
	// input is a percentage (/100) of the maximum amplitude
	if ( amp > 100 ){
		thrower ( "DDS amplitude out of range, should be < 100 %" );
	}
	return (unsigned) round ( amp * ( pow ( 2, 10 ) - 1 ) / 100.0 );
}

INT DdsCore::get32bitATW ( double amp ){
	// why do we need this and the getATW function?
	//SIGNED
	if ( abs ( amp ) > 100 ){
		thrower ( "ERROR: DDS amplitude out of range, should be < 100%." );
	}
	return (INT) round ( amp * ( pow ( 2, 32 ) - pow ( 2, 22 ) ) / 100.0 );
}

void DdsCore::longUpdate ( ){
	writeDDS ( 0, 0x1d, 0, 0, 0, 1 );
	writeDDS ( 1, 0x1d, 0, 0, 0, 1 );
}

void DdsCore::lockPLLs ( ){
	writeDDS ( 0, 1, 0, 0b10101000, 0, 0 );
	writeDDS ( 1, 1, 0, 0b10101000, 0, 0 );
	longUpdate ( );
	Sleep ( 100 ); //This delay is critical, need to give the PLL time to lock.
}

void DdsCore::channelSelect ( UINT8 device, UINT8 channel ){
	// ??? this is hard-coded...
	UINT8 CW = 0b11100000;
	//CW |= 1 << (channel + 4);
	writeDDS ( device, 0, 0, 0, 0, CW );
}


void DdsCore::writeAmpMultiplier ( UINT8 device ){
	// Necessary to turn on amplitude multiplier.
	UINT8 byte1 = 1 << 4;
	writeDDS ( device, 6, 0, 0, 0, 0 );
	writeDDS ( device, 6, 0, 0, byte1, 0 );
}

void DdsCore::writeResetFreq ( UINT8 device, UINT8 channel, double freq ){
	UINT16 address = RESET_FREQ_ADDR_OFFSET + 4 * device + 3 - channel;
	writeDDS ( WBWRITE_ARRAY, address, intTo4Bytes ( getFTW ( freq ) ) );
}

void DdsCore::writeResetAmp ( UINT8 device, UINT8 channel, double amp ){
	UINT16 address = RESET_AMP_ADDR_OFFSET + 4 * device + 3 - channel;
	writeDDS ( WBWRITE_ARRAY, address, intTo4Bytes ( getATW ( amp ) << 22 ) );
}

void DdsCore::writeRampReps ( UINT8 index, UINT16 reps ){
	UINT16 address = REPS_ADDRESS_OFFSET + index;
	UINT8 byte4 = reps & 0x000000ffUL;
	UINT8 byte3 = ( reps & 0x0000ff00UL ) >> 8;
	writeDDS ( WBWRITE_ARRAY, address, 0, 0, byte3, byte4 );
}

UINT16 DdsCore::getRepsFromTime ( double time ){
	// 125 kHz update rate
	// units of time is milliseconds
	double deltaTime = 8e-3; // 8 usec
	double maxTime = ( ( std::numeric_limits<UINT16>::max )( ) - 0.5 ) * deltaTime;
	unsigned __int64 repNum = ( time / deltaTime ) + 0.5;
	unsigned __int64 maxTimeLong = ( std::numeric_limits<UINT16>::max )( );
	// I'm moderately confused as to why I have to also exclude the max time itself here, this should be all 1's in 
	// binary.
	if ( repNum >= maxTimeLong ){
		thrower ( "time too long for ramp! Max time is " + str ( maxTime ) + " ms" );
	}
	return repNum;
}


void DdsCore::clearDdsRampMemory ( ){
	for ( auto rampI : range ( 255 ) ){
		writeRampReps ( rampI, 0 );
		for ( auto board : range ( 2 ) ){
			for ( auto chan : range ( 4 ) ){
				writeRampDeltaFreq ( board, chan, rampI, 0 );
				writeRampDeltaAmp ( board, chan, rampI, 0 );
			}
		}
	}
}


void DdsCore::writeRampDeltaFreq ( UINT8 device, UINT8 channel, UINT8 index, double deltafreq ){
	UINT16 address = RAMP_FREQ_ADDR_OFFSET + RAMP_CHANNEL_ADDR_SPACING * ( 4 * device + 3 - channel ) + index;
	writeDDS ( WBWRITE_ARRAY, address, intTo4Bytes ( getFTW ( deltafreq ) ) );
}

void DdsCore::writeRampDeltaAmp ( UINT8 device, UINT8 channel, UINT8 index, double deltaamp ){
	UINT16 address = RAMP_AMP_ADDR_OFFSET + RAMP_CHANNEL_ADDR_SPACING * ( 4 * device + 3 - channel ) + index;
	writeDDS ( WBWRITE_ARRAY, address, intTo4Bytes ( get32bitATW ( deltaamp ) ) );
}

std::array<UINT8, 4> DdsCore::intTo4Bytes ( int i_ ){
	// order here is hiword to loword, i.e. if you wrote out the int in 
	// This looks strange. FTW is an insigned int but it's being bitwise compared to an unsigned long. (the UL suffix)
	std::array<UINT8, 4> res;
	res[ 3 ] = ( i_ & 0x000000ffUL );
	res[ 2 ] = ( ( i_ & 0x0000ff00UL ) >> 8 );
	res[ 1 ] = ( ( i_ & 0x00ff0000UL ) >> 16 );
	res[ 0 ] = ( ( i_ & 0xff000000UL ) >> 24 );
	return res;
}

// a low level writebtn wrapper around the ftFlume writebtn
void DdsCore::writeDDS ( UINT8 DEVICE, UINT16 ADDRESS, std::array<UINT8, 4> data ){
	writeDDS ( DEVICE, ADDRESS, data[ 0 ], data[ 1 ], data[ 2 ], data[ 3 ] );
}


void DdsCore::writeDDS ( UINT8 DEVICE, UINT16 ADDRESS, UINT8 dat1, UINT8 dat2, UINT8 dat3, UINT8 dat4 ){
	if ( connType == ddsConnectionType::type::Async ){
		// None of these should be possible based on the data types of these args. 
		if ( DEVICE > 255 || ADDRESS > 65535 || dat1 > 255 || dat2 > 255 || dat3 > 255 || dat4 > 255 ){
			thrower ( "Error: DDS write out of range." );
		}
		UINT8 ADDRESS_LO = ADDRESS & 0x00ffUL;
		UINT8 ADDRESS_HI = ( ADDRESS & 0xff00UL ) >> 8;
		std::vector<unsigned char> input = { unsigned char ( WBWRITE + DEVICE ), ADDRESS_HI, ADDRESS_LO, dat1, dat2, dat3, dat4 };
		ftFlume.write ( input, MSGLENGTH );
	}
	else{
		// could probably take out other options entierly...
		thrower ( "Incorrect connection type, should be ASYNC" );
	}
}


ddsExpSettings DdsCore::getSettingsFromConfig ( ConfigStream& file ){
	ddsExpSettings settings;
	unsigned numRamps = 0;
	file >> settings.control;
	file >> numRamps;
	settings.ramplist = std::vector<ddsIndvRampListInfo> ( numRamps );

	for ( auto& ramp : settings.ramplist){
		file >> ramp.index >> ramp.channel >> ramp.freq1.expressionStr >> ramp.amp1.expressionStr
			 >> ramp.freq2.expressionStr >> ramp.amp2.expressionStr >> ramp.rampTime.expressionStr;
		file.get ( );
	}	
	return settings;
}


void DdsCore::writeRampListToConfig ( std::vector<ddsIndvRampListInfo> list, ConfigStream& file ){
	file << "/*Ramp List Size:*/ " << list.size ( );
	unsigned count = 0;
	for ( auto& ramp : list )
	{
		file << "\n/*Ramp List Element #" + str (++count) + "*/"
			 << "\n/*Index:*/\t\t" << ramp.index
			 << "\n/*Channel:*/\t" << ramp.channel
			 << "\n/*Freq1:*/\t\t" << ramp.freq1
			 << "\n/*Amp1:*/\t\t" << ramp.amp1
			 << "\n/*Freq2:*/\t\t" << ramp.freq2
			 << "\n/*Amp2:*/\t\t" << ramp.amp2
			 << "\n/*Ramp-Time:*/\t" << ramp.rampTime;
	}
}

void DdsCore::logSettings (DataLogger& log, ExpThreadWorker* threadworker){
}

void DdsCore::manualLoadExpRampList (std::vector< ddsIndvRampListInfo> ramplist) {
	expRampList = ramplist;
}

void DdsCore::loadExpSettings (ConfigStream& stream){
	//ddsExpSettings settings;
	//ConfigSystem::stdGetFromConfig (stream, *this, settings);
	//expRampList = settings.ramplist;
	//experimentActive = settings.control;
}





/**********************************************************************************************************/
bool DdsCore::isValidDDSName(std::string name)
{
	for (UINT ddsInc = 0; ddsInc < size_t(DDSGrid::total); ddsInc++)
	{
		if (getDDSIdentifier(name) != -1) {
			return true;
		}
	}
	return false;
}

int DdsCore::getDDSIdentifier(std::string name)
{
	for (UINT ddsInc = 0; ddsInc < size_t(DDSGrid::total); ddsInc++)
	{
		// check names set by user.
		std::transform(names[ddsInc].begin(), names[ddsInc].end(), names[ddsInc].begin(), ::tolower);
		if (name == names[ddsInc])
		{
			return ddsInc;
		}
		// check standard names which are always acceptable.
		if (name ==  "dds" +
			str(ddsInc / size_t(DDSGrid::numPERunit)) + "_" +
			str(ddsInc % size_t(DDSGrid::numPERunit)))
		{
			return ddsInc;
		}
	}
	// not an identifier.
	return -1;
}

void DdsCore::setNames(std::array<std::string, size_t(DDSGrid::total)> namesIn)
{
	names = std::move(namesIn);
}

std::string DdsCore::getName(int ddsNumber)
{
	return names[ddsNumber];
}

std::array<std::string, size_t(DDSGrid::total)> DdsCore::getName()
{
	return names;
}


/*******/
void DdsCore::resetDDSEvents()
{
	ddsCommandFormList.clear();
	ddsCommandList.clear();
	ddsSnapshots.clear();
	ddsChannelSnapshots.clear();
}

void DdsCore::prepareForce()
{
	// purposefully preserve ddsCommandFormList, for inExpCal
	sizeDataStructures(1);
}

void DdsCore::sizeDataStructures(unsigned variations)
{
	ddsCommandList.clear();
	ddsSnapshots.clear();
	ddsChannelSnapshots.clear();

	ddsCommandList.resize(variations);
	ddsSnapshots.resize(variations);
	ddsChannelSnapshots.resize(variations);
}

void DdsCore::initializeDataObjects(unsigned variationNum)
{
	ddsCommandFormList = std::vector<DdsCommandForm>(variationNum);
	sizeDataStructures(variationNum);
}

std::vector<DdsCommand> DdsCore::getDdsCommand(unsigned variation)
{
	if (ddsCommandList.size() - 1 < variation) {
		thrower("Aquiring DDS command out of range");
	}
	return ddsCommandList[variation];
}

void DdsCore::setDDSCommandForm(DdsCommandForm command)
{
	ddsCommandFormList.push_back(command);
	// you need to set up a corresponding trigger to tell the dacs to change the output at the correct time. 
	// This is done later on interpretation of ramps etc.
}

void DdsCore::handleDDSScriptCommand(DdsCommandForm command, std::string name, std::vector<parameterType>& vars)
{
	if (command.commandName != "ddsfreq:" &&
		command.commandName != "ddsamp:" &&
		command.commandName != "ddsamplinspace:" &&
		command.commandName != "ddsfreqlinspace:" &&
		command.commandName != "ddsrampamp:" &&
		command.commandName != "ddsrampfreq:")
	{
		thrower("ERROR: dds commandName not recognized!");
	}
	if (!isValidDDSName(name))
	{
		thrower("ERROR: the name " + name + " is not the name of a dds!");
	}
	// convert name to corresponding dac line.
	command.line = getDDSIdentifier(name);
	if (command.line == -1)
	{
		thrower("ERROR: the name " + name + " is not the name of a dds!");
	}
	setDDSCommandForm(command);
}

/*TODO: add calibration usuage*/
void DdsCore::calculateVariations(std::vector<parameterType>& variables, ExpThreadWorker* threadworker, 
	std::vector<calSettings>& calibrationSettings)
{
	std::vector<calResult> calibrations;
	for (auto& calset : calibrationSettings) {
		calibrations.push_back(calset.result);
	}
	UINT variations;
	variations = variables.front().keyValues.size();
	if (variations == 0) {
		variations = 1;
	}
	/// imporantly, this sizes the relevant structures.
	sizeDataStructures(variations);

	bool resolutionWarningPosted = false;
	bool nonIntegerWarningPosted = false;

	for (UINT variationInc = 0; variationInc < variations; variationInc++)
	{
		for (UINT eventInc = 0; eventInc < ddsCommandFormList.size(); eventInc++)
		{
			if (variationInc == 0) {
				// clear calibration usage for each command in the first var, for later check the proper usage of calibration
				for (auto calIdx : range(calibrationSettings.size())) {
					calibrations[calIdx].currentActive = false;
				}
			}

			DdsCommand tempEvent;
			tempEvent.line = ddsCommandFormList[eventInc].line;
			tempEvent.repeatId = ddsCommandFormList[eventInc].repeatId; // this set the repeatId for all steps below
			// Deal with time.
			if (ddsCommandFormList[eventInc].time.first.size() == 0)
			{
				// no variable portion of the time.
				tempEvent.time = ddsCommandFormList[eventInc].time.second;
			}
			else
			{
				double varTime = 0;
				for (auto variableTimeString : ddsCommandFormList[eventInc].time.first)
				{
					varTime += variableTimeString.evaluate(variables, variationInc, calibrations);
				}
				tempEvent.time = varTime + ddsCommandFormList[eventInc].time.second;
			}

			if (ddsCommandFormList[eventInc].commandName == "ddsamp:")
			{
				/// single point.
				////////////////
				// deal with amp
				tempEvent.amp = ddsCommandFormList[eventInc].initVal.evaluate(variables, variationInc, calibrations);
				tempEvent.endAmp = tempEvent.amp;
				tempEvent.rampTime = 0;
				tempEvent.freq = 0;
				ddsCommandList[variationInc].push_back(tempEvent);
			}
			else if (ddsCommandFormList[eventInc].commandName == "ddsfreq:")
			{
				/// single point.
				////////////////
				// deal with amp
				tempEvent.freq = ddsCommandFormList[eventInc].initVal.evaluate(variables, variationInc, calibrations);
				tempEvent.endFreq = tempEvent.freq;
				tempEvent.rampTime = 0;
				tempEvent.amp = 0;
				ddsCommandList[variationInc].push_back(tempEvent);
			}
			else if (ddsCommandFormList[eventInc].commandName == "ddslinspaceamp:")
			{
				// interpret ramp time command. I need to know whether it's ramping or not.
				double rampTime = ddsCommandFormList[eventInc].rampTime.evaluate(variables, variationInc, calibrations);
				/// many points to be made.
				// convert initValue and finalValue to doubles to be used 
				double initValue, finalValue;
				int numSteps;
				initValue = ddsCommandFormList[eventInc].initVal.evaluate(variables, variationInc, calibrations);
				// deal with final value;
				finalValue = ddsCommandFormList[eventInc].finalVal.evaluate(variables, variationInc, calibrations);
				// deal with numPoints
				numSteps = ddsCommandFormList[eventInc].numSteps.evaluate(variables, variationInc, calibrations);
				double rampInc = (finalValue - initValue) / double(numSteps);
				if ((fabs(rampInc) < ddsAmplResolution/*DDS_MAX_AMP / pow(2, 10)*/) && !resolutionWarningPosted)
				{
					resolutionWarningPosted = true;
					thrower("Warning: numPoints of " + str(numSteps) + " results in an amplitude ramp increment of "
						+ str(rampInc) + " is below the resolution of the ddss (which is " + str(10/*DDS_MAX_AMP*/) + "/2^10 = "
						+ str(ddsAmplResolution/*DDS_MAX_AMP / pow(2, 10)*/) + "). \r\n");
				}
				double timeInc = rampTime / double(numSteps);
				double initTime = tempEvent.time;
				double currentTime = tempEvent.time;
				double val = initValue;

				for (auto stepNum : range(numSteps))
				{
					tempEvent.amp = val;
					tempEvent.time = currentTime;
					tempEvent.endAmp = val;
					tempEvent.rampTime = 0;
					tempEvent.freq = 0;
					ddsCommandList[variationInc].push_back(tempEvent);
					currentTime += timeInc;
					val += rampInc;
				}
				// and get the final amp.
				tempEvent.amp = finalValue;
				tempEvent.time = initTime + rampTime;
				tempEvent.endAmp = finalValue;
				tempEvent.rampTime = 0;
				tempEvent.freq = 0;
				ddsCommandList[variationInc].push_back(tempEvent);
			}
			else if (ddsCommandFormList[eventInc].commandName == "ddslinspacefreq:")
			{
				// interpret ramp time command. I need to know whether it's ramping or not.
				double rampTime = ddsCommandFormList[eventInc].rampTime.evaluate(variables, variationInc, calibrations);
				/// many points to be made.
				// convert initValue and finalValue to doubles to be used 
				double initValue, finalValue;
				int numSteps;
				initValue = ddsCommandFormList[eventInc].initVal.evaluate(variables, variationInc, calibrations);
				// deal with final value;
				finalValue = ddsCommandFormList[eventInc].finalVal.evaluate(variables, variationInc, calibrations);
				// deal with numPoints
				numSteps = ddsCommandFormList[eventInc].numSteps.evaluate(variables, variationInc, calibrations);
				double rampInc = (finalValue - initValue) / double(numSteps);
				if ((fabs(rampInc) < ddsFreqResolution) && !resolutionWarningPosted)
				{
					resolutionWarningPosted = true;
					thrower("Warning: numPoints of " + str(numSteps) + " results in a ramp increment of "
						+ str(rampInc) + " is below the frequency resolution of the ddss (which is 500/2^32 = "
						+ str(ddsFreqResolution) + "). \r\n");
				}
				double timeInc = rampTime / double(numSteps);
				double initTime = tempEvent.time;
				double currentTime = tempEvent.time;
				double val = initValue;

				for (auto stepNum : range(numSteps))
				{
					tempEvent.freq = val;
					tempEvent.time = currentTime;
					tempEvent.endFreq = val;
					tempEvent.rampTime = 0;
					tempEvent.amp = 0;
					ddsCommandList[variationInc].push_back(tempEvent);
					currentTime += timeInc;
					val += rampInc;
				}
				// and get the final amp.
				tempEvent.freq = finalValue;
				tempEvent.time = initTime + rampTime;
				tempEvent.endFreq = finalValue;
				tempEvent.rampTime = 0;
				tempEvent.amp = 0;
				ddsCommandList[variationInc].push_back(tempEvent);
			}
			else if (ddsCommandFormList[eventInc].commandName == "ddsrampamp:")
			{
				double rampTime = ddsCommandFormList[eventInc].rampTime.evaluate(variables, variationInc, calibrations);
				// convert initValue and finalValue to doubles to be used 
				double initValue, finalValue, numSteps;
				initValue = ddsCommandFormList[eventInc].initVal.evaluate(variables, variationInc, calibrations);
				// deal with final value;
				finalValue = ddsCommandFormList[eventInc].finalVal.evaluate(variables, variationInc, calibrations);
				// set votlage resolution to be maximum allowed by the ramp range and time
				numSteps = rampTime / DDS_TIME_RESOLUTION;
				double rampInc = (finalValue - initValue) / numSteps;
				if ((fabs(rampInc) < ddsAmplResolution) && !resolutionWarningPosted)
				{
					resolutionWarningPosted = true;
					thrower("Warning: numPoints of " + str(numSteps) + " results in a ramp increment of "
						+ str(rampInc) + " is below the amplitude resolution of the dacs (which is " + str(10) + "/2^10 = "
						+ str(ddsAmplResolution) + "). Ramp will not run.\r\n");
				}
				if (numSteps > 65535) {
					thrower("Warning: numPoints of " + str(numSteps) + 
						" is larger than the max time of the DDS ramps. Ramp will be truncated. \r\n");
				}

				double initTime = tempEvent.time;

				// for ddsrampamp, pass the ramp points and time directly to a single ddsCommandList element
				tempEvent.amp = initValue;
				tempEvent.endAmp = finalValue;
				tempEvent.time = initTime;
				tempEvent.rampTime = rampTime;
				tempEvent.freq = 0;
				ddsCommandList[variationInc].push_back(tempEvent);
			}
			else if (ddsCommandFormList[eventInc].commandName == "ddsrampfreq:")
			{
				double rampTime = ddsCommandFormList[eventInc].rampTime.evaluate(variables, variationInc, calibrations);
				// convert initValue and finalValue to doubles to be used 
				double initValue, finalValue, numSteps;
				initValue = ddsCommandFormList[eventInc].initVal.evaluate(variables, variationInc, calibrations);
				// deal with final value;
				finalValue = ddsCommandFormList[eventInc].finalVal.evaluate(variables, variationInc, calibrations);
				// set votlage resolution to be maximum allowed by the ramp range and time
				numSteps = rampTime / DDS_TIME_RESOLUTION;
				double rampInc = (finalValue - initValue) / numSteps;
				if ((fabs(rampInc) < ddsFreqResolution) && !resolutionWarningPosted)
				{
					resolutionWarningPosted = true;
					thrower("Warning: numPoints of " + str(numSteps) + " results in a ramp increment of "
						+ str(rampInc) + " is below the frequency resolution of the ddss (which is 500/2^32 = "
						+ str(ddsFreqResolution) + "). Ramp will not run.\r\n");
				}
				if (numSteps > 65535) {
					thrower("Warning: numPoints of " + str(numSteps) + 
						" is larger than the max time of the DDS ramps. Ramp will be truncated. \r\n");
				}

				double initTime = tempEvent.time;

				// for ddsrampfreq, pass the ramp points and time directly to a single ddsCommandList element
				tempEvent.freq = initValue;
				tempEvent.endFreq = finalValue;
				tempEvent.time = initTime;
				tempEvent.rampTime = rampTime;
				tempEvent.amp = 0;
				ddsCommandList[variationInc].push_back(tempEvent);
			}
			else
			{
				thrower("ERROR: Unrecognized dds command name: " + ddsCommandFormList[eventInc].commandName);
			}
		}
	}
	for (size_t idx = 0; idx < calibrationSettings.size(); idx++) {
		calibrationSettings[idx].result = calibrations[idx];
	}
}


void DdsCore::constructRepeats(repeatManager& repeatMgr)
{
	typedef DdsCommand Command;
	/* this is to be done after ttlCommandList is filled with all variations. */
	if (ddsCommandFormList.size() == 0 || ddsCommandList.size() == 0) {
		thrower("No DAC Commands???");
	}

	unsigned variations = ddsCommandList.size();
	repeatMgr.saveCalculationResults(); // repeatAddedTime is changed during construction, need to save and reset it before and after the construction body
	auto* repearRoot = repeatMgr.getRepeatRoot();
	auto allDescendant = repearRoot->getAllDescendant();
	if (allDescendant.empty()) {
		return; // no repeats need to handle.
	}


	// iterate through all variations
	for (auto varInc : range(variations)) {
		auto& cmds = ddsCommandList[varInc];
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

bool DdsCore::repeatsExistInCommandForm(repeatInfoId repeatId)
{
	typedef DdsCommandForm CommandForm;
	auto& cmdFormList = ddsCommandFormList;
	auto cmdFormRepeated = std::find_if(cmdFormList.begin(), cmdFormList.end(),
		[&](const CommandForm& a) {
			return (a.repeatId.repeatIdentifier == repeatId.repeatIdentifier);
		});
	return (cmdFormRepeated != cmdFormList.end());
}

void DdsCore::addPlaceholderRepeatCommand(repeatInfoId repeatId)
{
	typedef DdsCommandForm CommandForm;
	auto& cmdFormList = ddsCommandFormList;
	repeatId.placeholder = true;
	CommandForm cmdForm;
	cmdForm.commandName = "ddsfreq:";
	cmdForm.initVal = Expression("0.0");
	cmdForm.repeatId = repeatId;
	cmdFormList.push_back(cmdForm); // should be an alert for all other commands, and it will be cleansed after advancing the time
}

void DdsCore::organizeDDSCommands(UINT variation)
{
	// each element of this is a different time (the double), and associated with each time is a vector which locates 
	// which commands were at this time, for
	// ease of retrieving all of the values in a moment.
	timeOrganizer.clear();

	std::vector<DdsCommand> tempEvents(ddsCommandList[variation]);
	// sort the events by time. using a lambda here.
	std::sort(tempEvents.begin(), tempEvents.end(),
		[](DdsCommand a, DdsCommand b) {return a.time < b.time; });
	for (UINT commandInc = 0; commandInc < tempEvents.size(); commandInc++)
	{
		// because the events are sorted by time, the time organizer will already be sorted by time, and therefore I 
		// just need to check the back value's time.
		if (commandInc == 0 || fabs(tempEvents[commandInc].time - timeOrganizer.back().first) > 2 * DBL_EPSILON)
		{
			// new time
			timeOrganizer.push_back({ tempEvents[commandInc].time,
									std::vector<DdsCommand>({ tempEvents[commandInc] }) });
		}
		else
		{
			// old time
			timeOrganizer.back().second.push_back(tempEvents[commandInc]);
		}
	}
	/// make the snapshots
	if (timeOrganizer.size() == 0)
	{
		// no commands, that's fine.
		return;
	}
}


void DdsCore::makeFinalDataFormat(UINT variation)
{
	DdsChannelSnapshot channelSnapshot;

	//for each channel with a changed amp or freq add a ddsSnapshot to the final list
	for (UINT commandInc = 0; commandInc < timeOrganizer.size(); commandInc++) {
		for (UINT zeroInc = 0; zeroInc < timeOrganizer[commandInc].second.size(); zeroInc++)
		{
			if (timeOrganizer[commandInc].second[zeroInc].freq == 0) {
				channelSnapshot.ampOrFreq = 'a'; // amp change
				channelSnapshot.val = timeOrganizer[commandInc].second[zeroInc].amp;
				channelSnapshot.endVal = timeOrganizer[commandInc].second[zeroInc].endAmp;
			}
			else
			{
				channelSnapshot.ampOrFreq = 'f'; //freq change
				channelSnapshot.val = timeOrganizer[commandInc].second[zeroInc].freq;
				channelSnapshot.endVal = timeOrganizer[commandInc].second[zeroInc].endFreq;
			}
			channelSnapshot.time = timeOrganizer[commandInc].first;
			channelSnapshot.channel = timeOrganizer[commandInc].second[zeroInc].line;

			channelSnapshot.rampTime = timeOrganizer[commandInc].second[zeroInc].rampTime;
			ddsChannelSnapshots[variation].push_back(channelSnapshot);
		}
	}
}


void DdsCore::standardExperimentPrep(UINT variation)
{
	organizeDDSCommands(variation);
	makeFinalDataFormat(variation);
}

void DdsCore::writeDDSs(UINT variation, bool loadSkip)
{

	//dioFPGA[variation].write();
	int tcp_connect;
	try
	{
		tcp_connect = zynq_tcp.connectTCP(ZYNQ_ADDRESS);
	}
	catch (ChimeraError& err)
	{
		tcp_connect = 1;
		thrower(err.what());
	}

	if (tcp_connect == 0)
	{
		zynq_tcp.writeDDSs(ddsChannelSnapshots[variation]);
		zynq_tcp.disconnect();
	}
	else
	{
		throw("connection to zynq failed. can't write DDS data\n");
	}
}

void DdsCore::setGUIDdsChange(std::vector<std::vector<DdsChannelSnapshot>> channelSnapShot)
{
	prepareForce();
	ddsChannelSnapshots = channelSnapShot;
	writeDDSs(0, true);
	//int tcp_connect;
	//try {
	//	tcp_connect = zynq_tcp.connectTCP(ZYNQ_ADDRESS);
	//}
	//catch (ChimeraError& err) {
	//	tcp_connect = 1;
	//	thrower(err.what());
	//}
	//Sleep(100);
	//if (tcp_connect == 0) {
	//	zynq_tcp.writeCommand("trigger");
	//	zynq_tcp.disconnect();
	//}
	//else {
	//	thrower("connection to zynq failed. can't write DDS data\n");
	//}
}
