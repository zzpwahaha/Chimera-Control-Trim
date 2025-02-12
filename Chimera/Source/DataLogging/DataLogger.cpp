// created by Mark O. Brown

#include "stdafx.h"
#include "DataLogger.h"
#include "RealTimeDataAnalysis/DataAnalysisControl.h"
#include "Andor/CameraImageDimensions.h"
#include "ExperimentThread/ExperimentThreadInput.h"
#include <ConfigurationSystems/ConfigSystem.h>
#include <ExperimentThread/autoCalConfigInfo.h>
#include <AnalogOutput/AoSystem.h>
#include <DirectDigitalSynthesis/DdsSystem.h>
#include <OffsetLock/OlSystem.h>
#include <Scripts/Script.h>
#include <ctime>

#include <filesystem>

DataLogger::DataLogger(std::string systemLocation, IChimeraQtWindow* parent) : IChimeraSystem(parent){
	// initialize this to false.
	fileIsOpen = false;
	dataFilesBaseLocation = systemLocation;
}

DataLogger::~DataLogger( ){
	// try to close before finishing.
	normalCloseFile( );
}

// this file assumes that h5 is the data_#.h5 file. User should check if incDataSet is on before calling. ???
void DataLogger::deleteFile(std::string fileName){
	if (fileIsOpen){
		// I'm not actually sure if this should be a prob with h5.
		thrower ("ERROR: Can't delete current h5 file, the h5 file is open!");
	}
	if (fileName.empty()) {
		fileName = "data_" + str(currentDataFileNumber);
	}
	std::string fileAddress = dataFilesBaseLocation + todayFolder + "\\Raw Data\\"
		+ fileName + ".h5";
	int success = DeleteFile(cstr(fileAddress));
	if (success == false){
		thrower ("Failed to delete h5 file! Error code: " + str(GetLastError()) + ".\r\n");
	}
	else{
		emit notification( "Deleted h5 file located at \"" + qstr(fileAddress) + "\"\r\n" );
	}
}


void DataLogger::getDataLocation ( std::string base, std::string& todayFolder, std::string& fullPath ){
	time_t timeInt = time ( 0 );
	struct tm timeStruct;
	localtime_s ( &timeStruct, &timeInt );
	std::string tempStr = str ( timeStruct.tm_year + 1900 );
	// Create the string of the date.
	std::string finalSaveFolder;
	finalSaveFolder += tempStr + "\\";
	std::string month;
	switch ( timeStruct.tm_mon + 1 ){
		case 1:  month = "January";		break;
		case 2:  month = "February";	break;
		case 3:  month = "March";		break;
		case 4:  month = "April";		break;
		case 5:  month = "May";			break; 
		case 6:  month = "June";		break;
		case 7:  month = "July";		break;
		case 8:  month = "August";		break; 
		case 9:  month = "September";	break;
		case 10: month = "October";		break; 
		case 11: month = "November";	break;
		case 12: month = "December";	break;
	}
	finalSaveFolder += month + "\\" + month + " " + str ( timeStruct.tm_mday );
	todayFolder = finalSaveFolder;
	// create date's folder.
	int result = 1;
	struct stat info;
	int resultStat = stat ( cstr ( base + finalSaveFolder ), &info );
	if ( resultStat != 0 ){
		//result = CreateDirectory ( cstr ( base + finalSaveFolder ), 0 );
		result = std::filesystem::create_directories((base + finalSaveFolder).c_str());
	}
	if ( !result ){
		thrower ( "ERROR: Failed to create save location for data at location " + base + finalSaveFolder +
				  ". Make sure you have access to the jilafile or change the save location. Error: " + str ( GetLastError ( ) )
				  + "\r\n" );
	}
	finalSaveFolder += "\\Raw Data";
	resultStat = stat ( cstr ( base + finalSaveFolder ), &info );
	if ( resultStat != 0 ){
		result = CreateDirectory ( cstr ( base + finalSaveFolder ), 0 );
	}
	if ( !result ){
		thrower ( "ERROR: Failed to create save location for data! Error: " + str ( GetLastError ( ) ) + "\r\n" );
	}
	finalSaveFolder += "\\";
	fullPath = finalSaveFolder;
}


int DataLogger::getCalibrationFileIndex () {
	unsigned count = 0;
	for (auto cal : AUTO_CAL_LIST) {
		std::string finalSaveFolder;
		getDataLocation (dataFilesBaseLocation, todayFolder, finalSaveFolder);
		FILE* calFile;
		auto calDataLoc = dataFilesBaseLocation + finalSaveFolder + cal.fileName + ".h5";
		fopen_s (&calFile, calDataLoc.c_str (), "r");
		if (!calFile) {
			return count;
		}
		else {
			// all good.
			fclose (calFile);
			count++;
		}
	}
	return -1; // all exist...
}


void DataLogger::assertCalibrationFilesExist (){
	for (auto cal : AUTO_CAL_LIST) {
		std::string finalSaveFolder;
		getDataLocation (dataFilesBaseLocation, todayFolder, finalSaveFolder);
		FILE* calFile;
		auto calDataLoc = dataFilesBaseLocation + finalSaveFolder + cal.fileName;
		fopen_s (&calFile, calDataLoc.c_str (), "r");
		if (!calFile) {
			thrower ("ERROR: The Data logger doesn't see the MOT calibration data for today in the data folder, "
				"location:" + calDataLoc + ". Please make sure that the mot is running okay and then "
				"run F11 before starting an experiment.");
		}
		else{
			// all good.
			fclose (calFile);
		}
	}
}


/*
specialName: optional, the name of the data file. This is a unique name, it will not be incremented. This is used
	for things like the mot size measurement or temperature measurements which are automated. The default is to use
	data_ as the name and add an incrementing number at the end of it.
*/
void DataLogger::initializeDataFiles( std::string specialName, bool checkForCalibrationFiles ){
	// if the function fails, the h5 file will not be open. If it succeeds, this will get set to true.
	fileIsOpen = false;
	/// First, create the folder for today's h5 data.
	// Get the date and use it to set the folder where this data run will be saved.
	std::string finalSaveFolder;
	getDataLocation ( dataFilesBaseLocation, todayFolder, finalSaveFolder );

	/// check that temperature data is being recorded.
	FILE *temperatureFile;
	auto temperatureDataLocation = dataFilesBaseLocation + finalSaveFolder + "Temperature_Data.csv";
	fopen_s ( &temperatureFile, temperatureDataLocation.c_str(), "r" );
	if ( !temperatureFile )	{
		//thrower ( "ERROR: The Data logger doesn't see the temperature data for today in the data folder, location:"
		//		  + temperatureDataLocation + ". Please make sure that the temperature logger is working correctly "
		//		  "before starting an experiment." );
	}
	else{
		fclose ( temperatureFile );
	}
	/// check that the mot calibration files have been recorded.
	if ( checkForCalibrationFiles )	{
		assertCalibrationFilesExist ();
	}

	/// Get a filename appropriate for the data
	std::string finalSaveFileName;
	if ( specialName == "" ){
		// the default option.
		unsigned fileNum = getNextFileIndex ( dataFilesBaseLocation + finalSaveFolder + "data_", ".h5" );
		// at this point a valid filename has been found.
		finalSaveFileName = "data_" + str ( fileNum ) + ".h5";
		// update this, which is used later to move the key file.
		currentDataFileNumber = fileNum;
	}
	else{
		finalSaveFileName = specialName + ".h5";
	}

 	try	{
		normalCloseFile ( );
		assertClosed ();
 		// create the file. H5F_ACC_TRUNC means it will overwrite files with the same name.
		file = H5::H5File( cstr( dataFilesBaseLocation + finalSaveFolder + finalSaveFileName ), H5F_ACC_TRUNC );
		fileIsOpen = true;
		//H5::Group ttlsGroup( file.createGroup( "/TTLs" ) );
	}
	catch (H5::Exception err) {
		auto fullE = getFullError (err);
		throwNested ( "ERROR: Failed to initialize HDF5 data file: " + err.getDetailMsg() + "; Full error:" + fullE);
	}
	currentAndorPicNumber = 0;
	for (auto nn : CameraInfo::allCams) {
		currentMakoPicNumber.insert({ nn,0 });
	}
	
}


void DataLogger::logPlotData ( std::string name ){
}

void DataLogger::logAoSystemSettings ( AoSystem& aoSys ){
	auto info = aoSys.getDacInfo ( );
	H5::Group AoSystemGroup ( file.createGroup ( "/Ao_System" ) );
	unsigned count = 0;
	for ( auto& output : info ){
		H5::Group indvOutput( AoSystemGroup.createGroup ( "Output_" + str(count++) ) );
		writeDataSet ( output.name, "Output_Name", indvOutput );
		writeDataSet ( output.note, "Note", indvOutput );
		writeDataSet ( output.currVal, "Value_At_Start", indvOutput );
		writeDataSet ( output.defaultVal, "Default_Value", indvOutput );
		writeDataSet ( output.minVal, "Minimum_Value", indvOutput );
		writeDataSet ( output.maxVal, "Maximum_Value", indvOutput );
	}
}

void DataLogger::logOlSystemSettings(OlSystem& olSys) {
	auto info = olSys.getOlInfo();
	H5::Group OlSystemGroup(file.createGroup("/Ol_System"));
	unsigned count = 0;
	for (auto& output : info) {
		H5::Group indvOutput(OlSystemGroup.createGroup("OffsetLock_" + str(count++)));
		writeDataSet(output.name, "OffsetLock_Name", indvOutput);
		writeDataSet(output.note, "Note", indvOutput);
		writeDataSet(output.currFreq, "Freq_At_Start", indvOutput);
		writeDataSet(output.defaultFreq, "Default_Freq", indvOutput);
		writeDataSet(output.minFreq, "Minimum_Freq", indvOutput);
		writeDataSet(output.maxFreq, "Maximum_Freq", indvOutput);
	}
}

void DataLogger::logDdsSystemSettings(DdsSystem& ddsSys) {
	auto info = ddsSys.getDdsInfo();
	H5::Group DdsSystemGroup(file.createGroup("/Dds_System"));
	unsigned count = 0;
	for (auto& output : info) {
		H5::Group indvOutput(DdsSystemGroup.createGroup("DDS_" + str(count++)));
		writeDataSet(output.name, "DDS_Name", indvOutput);
		writeDataSet(output.note, "Note", indvOutput);
		writeDataSet(output.currFreq, "Freq_At_Start", indvOutput);
		writeDataSet(output.defaultFreq, "Default_Freq", indvOutput);
		writeDataSet(output.minFreq, "Minimum_Freq", indvOutput);
		writeDataSet(output.maxFreq, "Maximum_Freq", indvOutput);
		writeDataSet(output.currAmp, "Amp_At_Start", indvOutput);
		writeDataSet(output.defaultAmp, "Default_Amp", indvOutput);
		writeDataSet(output.minAmp, "Minimum_Amp", indvOutput);
		writeDataSet(output.maxAmp, "Maximum_Amp", indvOutput);
	}
}

void DataLogger::logDoSystemSettings ( DoCore& doSys ){
	auto names = doSys.getAllNames ( );
	H5::Group DoSystemGroup ( file.createGroup ( "/Do_System" ) );
	H5::Group namesG (DoSystemGroup.createGroup ("Names"));
	unsigned count = 0;
	for ( auto& name : names ){		
		writeDataSet ( name, "Name_"+str(count++), namesG);
	}
}

void DataLogger::logFunctions( H5::Group& group ){
	H5::Group funcGroup( group.createGroup( "Functions" ) );
	try{
		// Re-add the entries back in and figure out which one is the current one.
		// ah, this is also windows, and should be changed to use qt functionality
		std::vector<std::string> names;
		std::string search_path = FUNCTIONS_FOLDER_LOCATION + "\\" + "*." + Script::FUNCTION_EXTENSION;
		WIN32_FIND_DATA fd;
		HANDLE hFind;
		hFind = FindFirstFile( cstr( search_path ), &fd );
		if ( hFind != INVALID_HANDLE_VALUE ){
			do{
				// if looking for folders
				if ( !(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) )
				{
					names.push_back( fd.cFileName );
				}
			} while ( FindNextFile( hFind, &fd ) );
			FindClose( hFind );
		}
		
		for ( auto name : names ){
			std::ifstream functionFile( FUNCTIONS_FOLDER_LOCATION + "\\" + name );
			std::stringstream stream;
			stream << functionFile.rdbuf( );
			std::string tempStr( stream.str( ) );
			H5::Group indvFunc( funcGroup.createGroup( name ) );
			writeDataSet( stream.str( ), name, indvFunc);
		}
	}
	catch ( H5::Exception& err ){
		throwNested ( "ERROR: Failed to log functions in HDF5 file: " + err.getDetailMsg( ) );
	}
}

void DataLogger::initOptimizationFile ( ){
	if ( optFile.is_open ( ) ){
		finOptimizationFile ( );
	}
	std::string todayFolder, finalFolder;
	DataLogger::getDataLocation ( DATA_SAVE_LOCATION, todayFolder, finalFolder );
	unsigned fileNum = getNextFileIndex ( DATA_SAVE_LOCATION + finalFolder + "Optimization_Results_", ".txt" );
	optFile.open( DATA_SAVE_LOCATION + finalFolder + "Optimization_Results_" + str ( fileNum ) + ".txt" );
}


void DataLogger::updateOptimizationFile ( std::string appendTxt ){
	if ( !optFile.is_open ( ) )	{
		thrower ( "Tried to update optimization record file, but file was closed!" );
	}
	optFile << appendTxt;
}

void DataLogger::finOptimizationFile ( ){
	optFile.close ( );
}

std::string DataLogger::getFullError (H5::Exception& err) {
	FILE* pFile;
	// note the "w", so this file is constantly overwritten.
	fopen_s (&pFile, "TempH5Log.txt", "w");
	if (pFile != 0) {
		err.printErrorStack (pFile);
		fclose (pFile);
	}
	std::ifstream readFile ("TempH5Log.txt");
	if (!readFile) {
		thrower ("Failed to get full HDF5 Error! Read file failed to open?!?");
	}
	std::stringstream buffer;
	buffer << readFile.rdbuf ();
	return buffer.str ();
}


void DataLogger::logError ( H5::Exception& err ){
	FILE * pFile;
	fopen_s ( &pFile, "H5ErrLog.txt", "a" );
	if ( pFile != 0 ){
		err.printErrorStack ( pFile );
		fclose ( pFile );
	}
}


/*
This function is for logging things that are readbtn from the configuration file and otherwise obtained inside the 
main experiment thread, but not associated with a particular device. Not much should be logged here. 
*/
void DataLogger::logMasterRuntime (const ExpRuntimeData& expRuntime){
	try{
		H5::Group runtimeGroup ( file.createGroup ( "/Master-Runtime" ) );
		writeDataSet (expRuntime.repetitions, "Repetitions", runtimeGroup );
		writeDataSet (expRuntime.mainOpts.repetitionFirst, "Repetitions-First", runtimeGroup);
		writeDataSet (expRuntime.mainOpts.randomizeVariations, "Randomize-Variation", runtimeGroup);
		logParameters (expRuntime.expParams, runtimeGroup );
	}
	catch ( ChimeraError& ){
		throwNested ( "ERROR: Failed to log master runtime in HDF5 file." );
	}
}


void DataLogger::logMasterInput( ExperimentThreadInput* input ){
	try{
		if ( input == nullptr){
			H5::Group runParametersGroup( file.createGroup( "/Master-Input:NA" ) );
			return;
		}
		H5::Group runParametersGroup( file.createGroup( "/Master-Input" ) );
		writeDataSet (input->profile.configuration, "Configuration", runParametersGroup);
		std::ifstream configF(input->profile.configFilePath());
		if (!configF.is_open()) {
			thrower("While trying to log the config file content from " + input->profile.configFilePath()
				+ ", the config file failed to open!");
		}
		std::string configBuf(str(configF.rdbuf()));
		writeDataSet(configBuf, "Configuration-Content", runParametersGroup);
		std::ifstream masterConfigF(MASTER_CONFIGURATION_FILE_ADDRESS);
		if (!masterConfigF.is_open()) {
			thrower("While trying to log the master config file content from " + MASTER_CONFIGURATION_FILE_ADDRESS
				+ ", the master config file failed to open!");
		}
		std::string masterConfigBuf(str(masterConfigF.rdbuf()));
		writeDataSet(masterConfigBuf, "Master-Configuration-Content", runParametersGroup);
		writeDataSet( true, "Run-Master", runParametersGroup );
		if ( true /*runmaster*/ ) {
			std::ifstream masterScript( ConfigSystem::getMasterAddressFromConfig( input->profile ) );
			if ( !masterScript.is_open( ) )	{
				thrower ( "ERROR: Failed to load master script for data logger!" );
			}
			std::string scriptBuf( str( masterScript.rdbuf( ) ) );
			writeDataSet( scriptBuf, "Master-Script", runParametersGroup);
			writeDataSet( ConfigSystem::getMasterAddressFromConfig(input->profile), "Master-Script-File-Address", 
						  runParametersGroup );
		}
		else{
			writeDataSet( "", "NA:Master-Script", runParametersGroup );
			writeDataSet( "", "NA:Master-Script-File-Address", runParametersGroup );
		}
		logFunctions( runParametersGroup );
		logAoSystemSettings ( input->aoSys );
		logOlSystemSettings ( input->olSys );
		logDdsSystemSettings( input->ddsSys);
		logDoSystemSettings ( input->ttls );
	}
	catch ( H5::Exception& err ){
		auto fullE = getFullError (err);
		throwNested ( "ERROR: Failed to log master parameters in HDF5 file: detail:" + err.getDetailMsg( )
			+ "; Full error:" + fullE);
	}
}


void DataLogger::logParameters( const std::vector<parameterType>& parameters, H5::Group& group ){
	H5::Group paramGroup = group.createGroup( "Parameters" );
	for ( auto& param : parameters ){
		try {
			group.openGroup (param.name + ";" + param.parameterScope);
			continue;
		}
		catch (H5::Exception&) {
		}
		// the combination of the name and scope should be enough to make a unique name for the group.
		//H5::Group indvParam = paramGroup.createGroup (param.name + "; " + param.parameterScope);
		H5::Group indvParam = paramGroup.createGroup (param.name + ";" + param.parameterScope);
		writeDataSet (param.name, "Name", indvParam);
		writeDataSet (param.parameterScope, "Scope", indvParam);
		writeDataSet (param.constantValue, "Constant Value", indvParam);
		writeDataSet (param.keyValues, "Key Values", indvParam);
		writeDataSet (param.shuffleIndex, "Shuffle Index", indvParam);
		writeDataSet (param.shuffleIndexReverse, "Shuffle Index Reverse", indvParam);
		writeDataSet (param.constant, "Is Constant", indvParam);
		writeDataSet (param.active, "Is Active", indvParam);
		writeDataSet (param.overwritten, "Is Overwritten", indvParam);
		writeDataSet (param.scanDimension, "Scan Dimension", indvParam);
		int rangeCount = 0;
		for (auto& range : param.ranges) {
			H5::Group rangeGroup = indvParam.createGroup ("Range " + str(rangeCount++));
			writeDataSet (range.initialValue, "Initial Value", rangeGroup);
			writeDataSet (range.finalValue, "Final Value", rangeGroup);
		}
	} 
}



void DataLogger::writeAndorPic( Matrix<long> image, imageParameters dims){
	if (fileIsOpen == false){
		thrower ("Tried to write to h5 file (for andor pic), but the file is closed!\r\n");
	}
	// MUST initialize status
	// starting coordinates of writebtn area in the h5 file of the array of picture data points.
	hsize_t offset[] = { currentAndorPicNumber++, 0, 0 };
	hsize_t slabdim[3] = { 1, dims.heightBinned(), dims.widthBinned() };
	try{
		if (AndorPicureSetDataSpace.getId () == -1) {
			hsize_t dims[3];
			auto fn = file.getFileName ();
			try {
				H5Sget_simple_extent_dims (AndorPicureSetDataSpace.getId (), dims, nullptr);
			}
			catch (H5::Exception &) {
				throwNested ("Failed to write andor pic data to HDF5 file! Filename: \"" + fn + "\", currentAndorPicNumber: "
					+ str (currentAndorPicNumber) + ", FAILED to get dims! Should be valid: " + str(andorDataSetShouldBeValid ));
			}
			thrower ("Invalid datapspace ID? Failed to write andor pic data to HDF5 file! Filename: \"" + fn + "\", currentAndorPicNumber: "
				+ str (currentAndorPicNumber) + ", dims: " + str (dims[0]) + "," + str (dims[1]) + "," + str (dims[2])+
				"Should be valid : " + str(andorDataSetShouldBeValid ));
		}
		AndorPicureSetDataSpace.selectHyperslab( H5S_SELECT_SET, slabdim, offset );
		AndorPictureDataset.write( image.data.data(), H5::PredType::NATIVE_LONG, AndorPicDataSpace, AndorPicureSetDataSpace );
	}
	catch (H5::Exception& err){
		auto fullE = getFullError (err);
		auto fn = file.getFileName ();
		throwNested ( "Failed to write andor pic data to HDF5 file! Filename: \""+ fn + "\", currentAndorPicNumber: " 
					  + str(currentAndorPicNumber-1) + ", Error: " + str(err.getDetailMsg()) + "\n""; Full error:" 
			+ fullE);
	}
}

void DataLogger::writeMakoPic(std::vector<double> image, int width, int height, CameraInfo::name name)
{
	if (fileIsOpen == false) {
		thrower("Tried to write to h5 file (for Mako pic), but the file is closed!\r\n");
	}
	// starting coordinates of writebtn area in the h5 file of the array of picture data points.
	hsize_t offset[] = { currentMakoPicNumber[name]++, 0, 0 };
	hsize_t slabdim[3] = { 1, height, width };// dims.height (), dims.width ()}; // dim0(row/height) first and then dim1(col/width)
	try {
		MakoPicureSetDataSpace[name].selectHyperslab(H5S_SELECT_SET, slabdim, offset);
		MakoPictureDataset[name].write(image.data(), H5::PredType::NATIVE_DOUBLE, MakoPicDataSpace[name],
			MakoPicureSetDataSpace[name]);
	}
	catch (H5::Exception& err) {
		auto fullE = getFullError(err);
		throwNested("Failed to write mako pic data to HDF5 file! Error: " + str(err.getDetailMsg()) + "\n"
			"; Full error:" + fullE);
	}
}

void DataLogger::writeTemperature(std::pair<std::vector<long long>, std::vector<double>> timedata, std::string identifier)
{
	if (fileIsOpen == false) {
		thrower("Tried to write to h5 file (for temperature), but the file is closed!\r\n");
	}
	try {
		H5::Group temperature;
		try {
			temperature = file.openGroup("TemperatureData");
		}
		catch (H5::Exception& e) {
			/* group does not exists, create it */
			temperature = file.createGroup("TemperatureData");
		}
		H5::Group subtemp = temperature.createGroup(identifier);
		writeDataSet(timedata.first, "TimeEpoch(ms)", subtemp);
		std::vector<std::string> timeStr;
		char buffer[128];
		std::transform(timedata.first.begin(), timedata.first.end(), std::back_inserter(timeStr), [&](long long epoch)-> std::string {
			strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", localtime(&epoch));
			return std::string(buffer); });
		writeDataSet(timeStr, "Datetime", subtemp);
		writeDataSet(timedata.second, "Temperature(K)", subtemp);
		
	}
	catch (H5::Exception& err) {
		auto fullE = getFullError(err);
		throwNested("ERROR: Failed to log Temperature data in HDF5 file: detail:" + err.getDetailMsg()
			+ "; Full error:" + fullE);
	}
}

void DataLogger::writePressure(std::pair<std::vector<long long>, std::vector<double>> timedata, std::string identifier, InfluxDataUnitType::mode unit)
{
	if (fileIsOpen == false) {
		thrower("Tried to write to h5 file (for pressure), but the file is closed!\r\n");
	}
	try {
		H5::Group temperature;
		try {
			temperature = file.openGroup("PressureData");
		}
		catch (H5::Exception& e) {
			/* group does not exists, create it */
			temperature = file.createGroup("PressureData");
		}
		H5::Group subtemp = temperature.createGroup(identifier);
		writeDataSet(timedata.first, "TimeEpoch(ms)", subtemp);
		std::vector<std::string> timeStr;
		char buffer[128];
		std::transform(timedata.first.begin(), timedata.first.end(), std::back_inserter(timeStr), [&](long long epoch)-> std::string {
			strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", localtime(&epoch));
			return std::string(buffer); });
		writeDataSet(timeStr, "Datetime", subtemp);
		switch (unit)
		{
		case InfluxDataUnitType::mode::K:
			thrower("Incorrect unit for Pressure in saveing influx monitor data!");
			break;
		case InfluxDataUnitType::mode::mBar:
			writeDataSet(timedata.second, "Pressure(mBar)", subtemp);
			break;
		case InfluxDataUnitType::mode::Torr:
			writeDataSet(timedata.second, "Pressure(Torr)", subtemp);
			break;
		}

	}
	catch (H5::Exception& err) {
		auto fullE = getFullError(err);
		throwNested("ERROR: Failed to log Pressure data in HDF5 file: detail:" + err.getDetailMsg()
			+ "; Full error:" + fullE);
	}
}
 
void DataLogger::writeVolts( unsigned currentVoltNumber, std::vector<float64> data ){
	if ( fileIsOpen == false ){
		thrower ( "Tried to write to h5 file, but the file is closed!\r\n" );
	}
	// starting coordinates of writebtn area in the h5 file of the array of picture data points.
	hsize_t offset[2] = { currentVoltNumber, 0 };
	hsize_t slabdim[2] = { 1, data.size() };
	try{
		voltsSetDataSpace.selectHyperslab( H5S_SELECT_SET, slabdim, offset );
		voltsDataSet.write( data.data( ), H5::PredType::NATIVE_DOUBLE, voltsDataSpace, voltsSetDataSpace);
	}
	catch ( H5::Exception& err ){
		auto fullE = getFullError (err);
		throwNested ( "Failed to write Analog voltage data to HDF5 file! Error: " + str( err.getDetailMsg( ) ) + "\n"
			"; Full error:" + fullE);
	}
} 

std::string DataLogger::getMostRecentDateString ( ){
	return mostRecentDateString;
}

void DataLogger::logMiscellaneousStart(){
	try	{
		H5::Group miscellaneousGroup ( file.createGroup ( "/Miscellaneous" ) );
		time_t t = time ( 0 );   // get time now
		struct tm now;
		localtime_s ( &now, &t );
		std::string monStr;
		switch ( now.tm_mon ){
			case 0: monStr = "January"; break;
			case 1: monStr = "February"; break;
			case 2: monStr = "March"; break;
			case 3: monStr = "April"; break;
			case 4: monStr = "May"; break;
			case 5: monStr = "June"; break;
			case 6: monStr = "July"; break;
			case 7: monStr = "August"; break;
			case 8: monStr = "September"; break;
			case 9: monStr = "October"; break;
			case 10: monStr = "November"; break;
			case 11: monStr = "December"; break;
		}
		mostRecentDateString = str ( now.tm_mday ) + "," + monStr + "," + str ( now.tm_year + 1900 );
		writeDataSet ( str ( now.tm_year + 1900 ) + "-" + str ( now.tm_mon + 1 ) + "-" + str ( now.tm_mday ),
					   "Start-Date", miscellaneousGroup );
		writeDataSet ( str ( now.tm_hour ) + ":" + str ( now.tm_min ) + ":" + str ( now.tm_sec ) + ":",
					   "Start-Time", miscellaneousGroup );
	}
	catch ( H5::Exception& err ){
		auto fullE = getFullError (err);
		throwNested ( "Failed to write miscellaneous start data to HDF5 file! Error: " + str ( err.getDetailMsg ( ) ) 
			+ "\n""; Full error:" + fullE);
	}
}


void DataLogger::assertClosed () {
	AndorPicureSetDataSpace.close ();
	AndorPictureDataset.close ();
	for (auto nn : CameraInfo::allCams) {
		MakoPictureDataset[nn].close();
		MakoPicureSetDataSpace[nn].close();
	}
	voltsDataSpace.close ();
	voltsDataSet.close ();
	file.close ();
	andorDataSetShouldBeValid = false;
	fileIsOpen = false;
	emit notification ("Closing HDF5 File and associated structures.\n", 0);
}

void DataLogger::normalCloseFile(){
	if (!fileIsOpen){
		return;
	}
	try	{
		// log the close time.
		H5::Group miscellaneousGroup ( file.openGroup ( "/Miscellaneous" ) );
		time_t t = time ( 0 );   // get time now
		struct tm now;
		localtime_s ( &now, &t );
		writeDataSet ( str ( now.tm_year + 1900 ) + "-" + str ( now.tm_mon + 1 ) + "-" + str ( now.tm_mday ),
					   "Stop-Date", miscellaneousGroup );
		writeDataSet ( str ( now.tm_hour ) + ":" + str ( now.tm_min ) + ":" + str ( now.tm_sec ) + ":",
					   "Stop-Time", miscellaneousGroup );
	}
	catch ( H5::Exception& err ){
		auto fullE = getFullError (err);
		thrower ("Normal Close Failed???; Full error:" + fullE);
	}
	assertClosed ();
}

 
unsigned DataLogger::getNextFileNumber(){
	return currentDataFileNumber+1;
}
 

int DataLogger::getDataFileNumber(){
	return currentDataFileNumber;
} 

/// simple wrappers for writing data sets
H5::DataSet DataLogger::writeDataSet( bool data, std::string name, H5::Group& group ){
	try	{
		hsize_t rank1[] = { 1 };
		H5::DataSet dset = group.createDataSet( cstr( name ), H5::PredType::NATIVE_HBOOL, H5::DataSpace( 1, rank1 ) );
		dset.write( &data, H5::PredType::NATIVE_HBOOL );
		return dset;
	}
	catch ( H5::Exception& err ){
		auto fullE = getFullError (err);
		throwNested ( "ERROR: error while writing bool data set to H5 File. bool was " + str( data )
				 + ". Dataset name was " + name + ". Error was :\r\n" + err.getDetailMsg( ) +
			"; Full error:" + fullE);
	}
}


H5::DataSet DataLogger::writeDataSet( unsigned __int64 data, std::string name, H5::Group& group ){
	try	{
		hsize_t rank1[] = { 1 };
		H5::DataSet dset = group.createDataSet( cstr( name ), H5::PredType::NATIVE_ULLONG, H5::DataSpace( 1, rank1 ) );
		dset.write( &data, H5::PredType::NATIVE_ULLONG );
		return dset;
	}
	catch ( H5::Exception& err ){
		auto fullE = getFullError (err);
		throwNested ( "ERROR: error while writing unsigned data set to H5 File. unsigned was " + str( data )
				 + ". Dataset name was " + name + ". Error was :\r\n" + err.getDetailMsg( ) + "; Full error:" + fullE);
	}
}


H5::DataSet DataLogger::writeDataSet( unsigned data, std::string name, H5::Group& group ){
	try	{
		hsize_t rank1[] = { 1 };
		H5::DataSet dset = group.createDataSet( cstr( name ), H5::PredType::NATIVE_UINT, H5::DataSpace( 1, rank1 ) );
		dset.write( &data, H5::PredType::NATIVE_UINT );
		return dset;
	}
	catch ( H5::Exception& err ){
		auto fullE = getFullError (err);
		throwNested ( "ERROR: error while writing unsigned data set to H5 File. unsigned was " + str( data )
				 + ". Dataset name was " + name + ". Error was :\r\n" + err.getDetailMsg( ) + "; Full error:" + fullE);
	}
}

H5::DataSet DataLogger::writeDataSet( int data, std::string name, H5::Group& group ){
	try	{
		hsize_t rank1[] = { 1 };
		H5::DataSet dset = group.createDataSet( cstr( name ), H5::PredType::NATIVE_INT, H5::DataSpace( 1, rank1 ) );
		dset.write( &data, H5::PredType::NATIVE_INT );
		return dset;
	}
	catch ( H5::Exception& err ){
		auto fullE = getFullError (err);
		throwNested ( "ERROR: error while writing int data set to H5 File. int was " + str( data )
				 + ". Dataset name was " + name + ". Error was :\r\n" + err.getDetailMsg( ) + "; Full error:" + fullE);
	}
}

H5::DataSet DataLogger::writeDataSet( double data, std::string name, H5::Group& group ){
	try{
		hsize_t rank1[] = { 1 };
		H5::DataSet dset = group.createDataSet( cstr( name ), H5::PredType::NATIVE_DOUBLE, H5::DataSpace( 1, rank1 ) );
		dset.write( &data, H5::PredType::NATIVE_DOUBLE );
		return dset;
	}
	catch ( H5::Exception& err ){
		auto fullE = getFullError (err);
		throwNested ( "ERROR: error while writing double data set to H5 File. double was " + str(data)
				 + ". Dataset name was " + name + ". Error was :\r\n" + err.getDetailMsg( ) + "; Full error:" + fullE);
	}
}

H5::DataSet DataLogger::writeDataSet( std::vector<float> dataVec, std::string name, H5::Group& group ){
	try	{
		hsize_t rank1[] = { 1 };
		rank1[0] = dataVec.size( );
		H5::DataSet dset = group.createDataSet( cstr( name ), H5::PredType::NATIVE_FLOAT, H5::DataSpace( 1, rank1 ) );
		// get from the key file
		dset.write( dataVec.data( ), H5::PredType::NATIVE_FLOAT );
		return dset;
	}
	catch ( H5::Exception& err ){
		auto fullE = getFullError (err);
		throwNested ( "ERROR: error while writing double float data set to H5 File. Dataset name was " + name
				 + ". Error was :\r\n" + err.getDetailMsg( ) + "; Full error:" + fullE);
	}
}


H5::DataSet DataLogger::writeDataSet( std::vector<double> dataVec, std::string name, H5::Group& group ){
	try	{
		hsize_t rank1[] = { 1 };
		rank1[0] = dataVec.size( );
		H5::DataSet dset = group.createDataSet( cstr( name ), H5::PredType::NATIVE_DOUBLE, H5::DataSpace( 1, rank1 ) );
		// get from the key file
		dset.write( dataVec.data( ), H5::PredType::NATIVE_DOUBLE );
		return dset;
	}
	catch ( H5::Exception& err ){
		auto fullE = getFullError (err);
		throwNested ( "ERROR: error while writing double vector data set to H5 File. Dataset name was " + name
				 + ". Error was :\r\n" + err.getDetailMsg( ) + "; Full error:" + fullE);
	}
}

H5::DataSet DataLogger::writeDataSet(std::vector<long long> dataVec, std::string name, H5::Group& group) {
	try {
		hsize_t rank1[] = { 1 };
		rank1[0] = dataVec.size();
		H5::DataSet dset = group.createDataSet(cstr(name), H5::PredType::NATIVE_LLONG, H5::DataSpace(1, rank1));
		// get from the key file
		dset.write(dataVec.data(), H5::PredType::NATIVE_LLONG);
		return dset;
	}
	catch (H5::Exception& err) {
		auto fullE = getFullError(err);
		throwNested("ERROR: error while writing long long vector data set to H5 File. Dataset name was " + name
			+ ". Error was :\r\n" + err.getDetailMsg() + "; Full error:" + fullE);
	}
}

H5::DataSet DataLogger::writeDataSet(std::vector<unsigned __int64> dataVec, std::string name, H5::Group& group) {
	try {
		hsize_t rank1[] = { 1 };
		rank1[0] = dataVec.size();
		H5::DataSet dset = group.createDataSet(cstr(name), H5::PredType::NATIVE_ULLONG, H5::DataSpace(1, rank1));
		// get from the key file
		dset.write(dataVec.data(), H5::PredType::NATIVE_ULLONG);
		return dset;
	}
	catch (H5::Exception& err) {
		auto fullE = getFullError(err);
		throwNested("ERROR: error while writing unsigned long long vector data set to H5 File. Dataset name was " + name
			+ ". Error was :\r\n" + err.getDetailMsg() + "; Full error:" + fullE);
	}
}

H5::DataSet DataLogger::writeDataSet( std::string data, std::string name, H5::Group& group ){
	try	{
		hsize_t rank1[] = { data.length( ) };
		H5::DataSet dset = group.createDataSet( cstr( name ), H5::PredType::C_S1, H5::DataSpace( 1, rank1 ) );
		dset.write( cstr( data ), H5::PredType::C_S1 );
		return dset;
	}
	catch ( H5::Exception& err ){
		auto fullE = getFullError (err);
		throwNested ( "ERROR: error while writing string data set to H5 File. String was " + data
				 + ". Dataset name was " + name + ". Error was :\r\n" + err.getDetailMsg( ) + "; Full error:" + fullE);
	}
}

void DataLogger::writeAttribute( double data, std::string name, H5::DataSet& dset ){
	try	{
		hsize_t rank1[] = { 1 };
		H5::Attribute attr = dset.createAttribute( cstr( name ), H5::PredType::NATIVE_DOUBLE, H5::DataSpace( 1, rank1 ) );
		attr.write( H5::PredType::NATIVE_DOUBLE, &data );
	}
	catch ( H5::Exception& err ){
		auto fullE = getFullError (err);
		throwNested ( "ERROR: error while writing bool attribute to H5 File. bool was " + str( data )
				 + ". Dataset name was " + name + ". Error was :\r\n" + err.getDetailMsg( ) + "; Full error:" + fullE);
	}
}

void DataLogger::writeAttribute( bool data, std::string name, H5::DataSet& dset ){
	try	{
		hsize_t rank1[] = { 1 };
		H5::Attribute attr = dset.createAttribute( cstr(name), H5::PredType::NATIVE_HBOOL, H5::DataSpace( 1, rank1 ) );
		attr.write( H5::PredType::NATIVE_HBOOL, &data);
	}
	catch ( H5::Exception& err ){
		auto fullE = getFullError (err);
		throwNested ( "ERROR: error while writing bool attribute to H5 File. bool was " + str(data)
				 + ". Dataset name was " + name + ". Error was :\r\n" + err.getDetailMsg( ) + "; Full error:" + fullE);
	}
}


/* As of right now just append all the strings and insert a newline between. There should be a nice way of doing this.*/
H5::DataSet DataLogger::writeDataSet ( std::vector<std::string> dataVec, std::string name, H5::Group& group ){
	std::string superString = "";
	for ( auto string : dataVec ){
		superString += string + "\n";
	}
	return writeDataSet ( superString, name, group );
}

