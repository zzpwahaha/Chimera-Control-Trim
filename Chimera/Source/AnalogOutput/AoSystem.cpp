// created by Mark O. Brown
#include "stdafx.h"
#include "AnalogOutput/AoSystem.h"
#include "PrimaryWindows/QtAuxiliaryWindow.h"
#include "PrimaryWindows/QtMainWindow.h"
#include "ConfigurationSystems/Version.h"
#include "ConfigurationSystems/ConfigSystem.h"
#include "GeneralUtilityFunctions/range.h"
#include <boost/lexical_cast.hpp>
#include <ExperimentThread/ExpThreadWorker.h>
#include <AnalogInput/calInfo.h>

#include <qlayout.h>

AoSystem::AoSystem(IChimeraQtWindow* parent) 
	: IChimeraSystem(parent)
	, core()
	, roundToDacPrecision(false)
	, dacTriggerTime(0.0005)
{ }


void AoSystem::zeroDacs()
{
	for ( int dacInc : range(size_t(AOGrid::total)) )
	{
		outputs[dacInc].info.currVal = 0.0;
	}
	//updateEdits();
	setDACs();
	emit notification ("Zero'd Analog Outputs.\n", 2);
}


std::array<AoInfo, size_t(AOGrid::total)> AoSystem::getDacInfo( ){
	std::array<AoInfo, size_t(AOGrid::total)> info;
	for ( auto dacNum : range(outputs.size()) ){
		info[dacNum] = outputs[dacNum].info;
	}
	return info;
}




void AoSystem::handleOpenConfig(ConfigStream& openFile)
{
	std::string test;
	for (auto& out : outputs) {
		openFile >> out.info.name;
	}
	try {
		for (auto& out : outputs) {
			openFile >> test;
			out.info.currVal = boost::lexical_cast<double>(test);
		}
		for (auto& out : outputs) {
			openFile >> test;
			out.info.maxVal = boost::lexical_cast<double>(test);
		}
		for (auto& out : outputs) {
			openFile >> test;
			out.info.minVal = boost::lexical_cast<double>(test);
		}
	}
	catch (boost::bad_lexical_cast&) {
		throwNested("AO control failed to convert values recorded in the config file "
			"to doubles");
	}
	for (auto& out : outputs) {
		openFile >> out.info.note;
	}
	std::array<std::string, size_t(AOGrid::total)> namesIn;
	for (unsigned idx = 0; idx < namesIn.size(); idx++)
	{
		namesIn[idx] = outputs[idx].info.name;
	}
	core.setNames(namesIn);
	updateEdits();
	setDACs();
}

void AoSystem::handleSaveConfig(ConfigStream& saveFile) 
{
	saveFile << core.getDelim() << "\n";
	saveFile << "/*DAC Name:*/ ";
	for (auto& out : outputs) {
		saveFile << out.info.name << " ";
	}
	saveFile << "\n";

	saveFile << "/*DAC Value:*/ ";
	for (auto& out : outputs) {
		saveFile << out.info.currVal << " ";
	}
	saveFile << "\n";

	saveFile << "/*DAC Value Max:*/ ";
	for (auto& out : outputs) {
		saveFile << out.info.maxVal << " ";
	}
	saveFile << "\n";

	saveFile << "/*DAC Value Min:*/ ";
	for (auto& out : outputs) {
		saveFile << out.info.minVal << " ";
	}
	saveFile << "\n";

	saveFile << "/*DAC Description:*/ ";
	for (auto& out : outputs) {
		saveFile << out.info.note << " ";
	}
	saveFile << "\nEND_" + core.getDelim() << "\n";
}

void AoSystem::standardExperimentPrep ( unsigned variationInc, std::vector<parameterType>& expParams, 
										double currLoadSkipTime ){
	core.organizeDacCommands(variationInc, { ZYNQ_DEADTIME,getDacValues() });
	core.findLoadSkipSnapshots (currLoadSkipTime, expParams, variationInc);
	//makeFinalDataFormat (variationInc);
	core.formatDacForFPGA(variationInc, { ZYNQ_DEADTIME,getDacValues() });
}





/// //////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// 
/// //////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void AoSystem::handleEditChange ( unsigned dacNumber ){
	if ( dacNumber >= outputs.size ( ) ){
		thrower ( "attempted to handle dac edit change, but the dac number reported doesn't exist!" );
	}
	outputs[ dacNumber ].handleEdit ( roundToDacPrecision );
}




void AoSystem::setDefaultValue(unsigned dacNum, double val) {
	outputs[dacNum].info.defaultVal = val;
}



double AoSystem::getDefaultValue(unsigned dacNum){
	return outputs[dacNum].info.defaultVal;
}

void AoSystem::initialize( IChimeraQtWindow* parent )
{
	QVBoxLayout* layout = new QVBoxLayout(this);
	this->setMaximumWidth(1000);
	// title

	dacTitle = new QLabel ("ANALOG OUTPUT", parent);
	layout->addWidget(dacTitle, 0);
	
	QHBoxLayout* layout1 = new QHBoxLayout();
	layout1->setContentsMargins(0, 0, 0, 0);

	dacSetButton = new CQPushButton ("Set New DAC Values", parent);
	dacSetButton->setToolTip("Press this button to attempt force all DAC values to the values currently recorded in the"
							 " edits below.");
	parent->connect (dacSetButton, &QPushButton::released, [parent]() {parent->auxWin->SetDacs (); });
	zeroDacsButton = new CQPushButton ("Zero DACs", parent);
	zeroDacsButton->setToolTip( "Press this button to set all dac values to zero." );
	parent->connect (zeroDacsButton, &QPushButton::released, [parent]() { parent->auxWin->zeroDacs(); });
	// 
	quickChange = new CQCheckBox ("Quick-Change", parent);
	quickChange->setChecked(false);
	quickChange->setToolTip ( "With this checked, you can quickly change a DAC's value by using the arrow keys while "
							 "having the cursor before the desired digit selected in the DAC's edit.");

	layout1->addWidget(dacSetButton, 0);
	layout1->addWidget(zeroDacsButton, 0);
	layout1->addWidget(quickChange, 0);
	layout->addLayout(layout1);
	

	QGridLayout* AOGridLayout = new QGridLayout();
	unsigned runningCount = 0;
	std::array<std::string, size_t(AOGrid::total)> dacNames;
	for ( auto& out : outputs )	
	{
		out.initialize ( parent, runningCount);
		AOGridLayout->addLayout(out.getLayout(), 
			runningCount % size_t(AOGrid::numPERunit), 
			runningCount / size_t(AOGrid::numPERunit));
		dacNames[runningCount] = out.info.name;
		runningCount++;
		
	}
	layout->addLayout(AOGridLayout);
	core.setNames(dacNames);

}

bool AoSystem::eventFilter (QObject* obj, QEvent* event){
	for (auto& out : outputs) {
		if ( out.eventFilter (obj, event) && quickChange->isChecked()) 
		{
			parentWin->auxWin->SetDacs();
			return true;
		}
	}
	return false;
}

/*TODO, think whether need this*/
void AoSystem::handleRoundToDac( )
{
	if ( roundToDacPrecision )
	{
		roundToDacPrecision = false;
		//mainWin->checkAllMenus ( ID_MASTER_ROUNDDACVALUESTODACPRECISION, MF_UNCHECKED );
	}
	else
	{
		roundToDacPrecision = true;
		//mainWin->checkAllMenus ( ID_MASTER_ROUNDDACVALUESTODACPRECISION, MF_CHECKED );
	}
}


/*
 * get the text from every edit and prepare a change. If fails to get text from edit, if useDefalt this will set such
 * dacs to zero.
 */
void AoSystem::handleSetDacsButtonPress( bool useDefault )
{
	for (auto dacInc : range(outputs.size()) )
	{
		handleEditChange(dacInc);
	}
	setDACs();
}


void AoSystem::updateEdits( )
{
	for ( auto& dac : outputs )
	{
		dac.updateEdit ( roundToDacPrecision );
	}
}



/*
 * IMPORTANT: this does not actually change any of the outputs of the board. It is meant to be called when things have
 * happened such that the control doesn't know what it's own status is, e.g. at the end of an experiment, since the 
 * program doesn't change it's internal memory of all of the status of the aoSys as the experiment runs. (it can't, 
 * besides it would intensive to keep track of that in real time).
 */
/*TODO get it a better name*/
void AoSystem::setDacStatusNoForceOut(std::array<double, size_t(AOGrid::total)> status)
{
	// set the internal values
	for ( auto outInc : range(outputs.size()) )
	{
		outputs[outInc].info.currVal = status[ outInc ];
		outputs[ outInc ].updateEdit ( roundToDacPrecision );
	}
}



// an "alias template". effectively a local "using std::vector;" declaration. Better not use in case confusion
//template<class T> using vec = std::vector<T>;



/*TODO get it a better name, the functioning is overlapped with setDacStatusNoForceOut*/
/// used in AuxWindow->handleMasterConfigOpen, address it later
// this is a function called in preparation for forcing a dac change. Remember, you need to call ___ to actually change things.
/*mainly for preparing the trigger which is not needed in zynq*/
void AoSystem::prepareDacForceChange(int line, double voltage)
{
	// change parameters in the AoSystem object so that the object knows what the current settings are.
	//std::string valStr = roundToDacPrecision? str ( AnalogOutput::roundToDacResolution ( voltage ), 13 ) : str ( voltage, 13 );
	//if (valStr.find(".") != std::string::npos)	{
	//	// then it's a double. kill extra zeros on the end.
	//	valStr.erase(valStr.find_last_not_of('0') + 1, std::string::npos);
	//}


	outputs[ line ].info.currVal = voltage;
	//dacValues[line] = voltage;

}



void AoSystem::setDACs()
{
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
		std::ostringstream stringStream;
		std::string command;
		for (int line = 0; line < size_t(AOGrid::total); ++line) 
		{
			stringStream.str("");
			stringStream << "DAC_" << line << "_" << 
				std::fixed << std::setprecision(numDigits) << outputs[line].info.currVal;
			command = stringStream.str();
			zynq_tcp.writeCommand(command);
		}
		zynq_tcp.disconnect();
	}
	else
	{
		thrower("connection to zynq failed. can't trigger the sequence or new settings\n");
	}
}

/*used only in the calibrationThread, so don't need gui update*/
void AoSystem::setSingleDac(unsigned dacNumber, double val)
{
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
	/*update outputs first and do a standard, single dac command*/
	outputs[dacNumber].info.currVal = val;
	//outputs[dacNumber].updateEdit();
	if (tcp_connect == 0)
	{
		std::ostringstream stringStream;
		std::string command;
		stringStream.str("");
		stringStream << "DAC_" << dacNumber << "_" <<
			std::fixed << std::setprecision(numDigits) << outputs[dacNumber].info.currVal;
		command = stringStream.str();
		zynq_tcp.writeCommand(command);
		zynq_tcp.disconnect();
	}
	else
	{
		thrower("connection to zynq failed. can't trigger the sequence or new settings\n");
	}

}


void AoSystem::setMinMax(int dacNumber, double minv, double maxv){
	if (!(minv <= maxv)){
		thrower ("Min dac value must be less than max dac value.");
	}
	if (minv < -10 || minv > 10 || maxv < -10 || maxv > 10)	{
		thrower ("Min and max dac values must be withing [-10,10].");
	}
	outputs[dacNumber].info.minVal = minv;
	outputs[ dacNumber ].info.maxVal = maxv;
}


std::pair<double, double> AoSystem::getDacRange(int dacNumber){
	return { outputs[ dacNumber ].info.minVal, outputs[ dacNumber ].info.maxVal };
}


void AoSystem::setName(int dacNumber, std::string name){
	outputs[ dacNumber ].setName ( name );
}


std::string AoSystem::getNote ( int dacNumber ){
	return outputs[dacNumber].info.note;
}


void AoSystem::setNote( int dacNum, std::string note ){
	outputs[ dacNum ].setNote ( note );
}


std::string AoSystem::getName(int dacNumber){
	return outputs[dacNumber].info.name;
}


unsigned AoSystem::getNumberOfDacs(){
	return outputs.size ( );
}


double AoSystem::getDacValue(int dacNumber){
	return outputs[dacNumber].info.currVal;
}

std::array<double, size_t(AOGrid::total)> AoSystem::getDacValues()
{
	std::array<double, size_t(AOGrid::total)> dacValueTmp;
	for (size_t i = 0; i < size_t(AOGrid::total); i++)
	{
		dacValueTmp[i] = getDacValue(i);
	}
	return dacValueTmp;
}


