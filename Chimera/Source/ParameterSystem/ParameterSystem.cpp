// created by Mark O. Brown
#include "stdafx.h"
#include "ParameterSystem.h"
#include "Scripts/Script.h"
#include "DigitalOutput/DoSystem.h"
#include "PrimaryWindows/IChimeraQtWindow.h"
#include <PrimaryWindows/QtScriptWindow.h>
#include "GeneralObjects/multiDimensionalKey.h"
#include "GeneralUtilityFunctions/cleanString.h"
#include <ConfigurationSystems/ConfigSystem.h>
#include <QHeaderView.h>
#include <iomanip>
#include <unordered_map>
#include <random>
#include <boost/lexical_cast.hpp>
#include <qmenu.h>
#include <GeneralObjects/ChimeraStyleSheets.h>


ParameterSystem::ParameterSystem(IChimeraQtWindow* parent, std::string configurationFileDelimiter) :
	IChimeraSystem(parent), configDelim(configurationFileDelimiter),
	paramModel(configurationFileDelimiter == "GLOBAL_PARAMETERS"),
	dragMoveEnable(false){ }

void ParameterSystem::handleContextMenu (const QPoint& pos){
	auto index = parametersView->indexAt (pos);
	QMenu menu;
	menu.setStyleSheet (chimeraStyleSheets::stdStyleSheet ());
	auto* deleteAction = new QAction ("Delete This Parameter", &menu);
	parametersView->connect (deleteAction, &QAction::triggered,
		[this, index]() {
			auto params = this->paramModel.getParams ();
			params.erase (params.begin () + index.row ());
			paramModel.setParams (params);
		});
	auto* newParam = new QAction ("New Parameter", &menu);
	parametersView->connect (newParam, &QAction::triggered,
		[this]() {
			auto params = paramModel.getParams ();
			params.push_back (parameterType (paramModel.getRangeInfo ().dimensionInfo (0).size ()));
			paramModel.setParams (params);
		});

	auto* toggleInclusivity = new QAction ("Toggle Inclusivity", &menu);
	parametersView->connect (toggleInclusivity, &QAction::triggered,
		[this, index]() {
			auto rangeInfo = paramModel.getRangeInfo ();
			int whichRange = (index.column () - paramModel.preRangeColumns) / 3;
			auto& range = rangeInfo (paramModel.getParams ()[index.row ()].scanDimension, whichRange);
			switch ((index.column () - paramModel.preRangeColumns) % 3) {
			case 0:
				range.leftInclusive = !range.leftInclusive;
				break;
			case 1:
				range.rightInclusive = !range.rightInclusive;
				break;
			}
			paramModel.setRangeInfo (rangeInfo);
		});

	auto* addRange = new QAction ("Add Range", &menu);
	parametersView->connect (addRange, &QAction::triggered, [this, index]() {
		auto rangeInfo = paramModel.getRangeInfo ();
		auto scanDim = paramModel.getParams ()[index.row ()].scanDimension;
		auto currVarRangeNum = paramModel.getParams()[index.row()].ranges.size();
		paramModel.setVariationRangeNumber (rangeInfo.numRanges (scanDim) + 1, currVarRangeNum, scanDim);
		setTableviewColumnSize();
		});
	auto* rmRange = new QAction ("Remove Range", &menu);
	parametersView->connect (rmRange, &QAction::triggered, [this, index]() {
		auto rangeInfo = paramModel.getRangeInfo ();
		auto scanDim = paramModel.getParams ()[index.row ()].scanDimension;
		auto currVarRangeNum = paramModel.getParams()[index.row()].ranges.size();
		paramModel.setVariationRangeNumber (rangeInfo.numRanges (scanDim) - 1, currVarRangeNum, scanDim);
		setTableviewColumnSize();
		});
	// change the selection type from item to rows, so that the drag and drop can work properly, 
	// having it checked shouldn't affect anything other than that the whole row will be selceted when click on one item
	auto* dragDropTgl = new QAction("Toggle Drag & Drop", &menu);
	dragDropTgl->setCheckable(true);
	dragDropTgl->setChecked(dragMoveEnable);
	parametersView->connect(dragDropTgl, &QAction::triggered, [this, dragDropTgl]() {
		dragMoveEnable = dragDropTgl->isChecked();
		if (dragMoveEnable) {
			parametersView->setDragEnabled(true);
			parametersView->setAcceptDrops(true);
			parametersView->setDropIndicatorShown(true);
			parametersView->setDragDropMode(QAbstractItemView::DragDrop);
			parametersView->setDragDropOverwriteMode(false);
			parametersView->setDefaultDropAction(Qt::MoveAction);
			parametersView->setSelectionMode(QAbstractItemView::ExtendedSelection);
			parametersView->setSelectionBehavior(QAbstractItemView::SelectRows);
		}
		else {
			parametersView->setDragEnabled(false);
			parametersView->setAcceptDrops(false);
			parametersView->setSelectionMode(QAbstractItemView::SingleSelection);
			parametersView->setSelectionBehavior(QAbstractItemView::SelectItems);
		}
		});

	//for (auto* action : {addRange, rmRange, deleteAction, toggleInclusivity, newParam}){
	//	parametersView->connect (action, &QAction::triggered, 
	//							 (IChimeraQtWindow*)(parametersView->parentWidget()), &IChimeraQtWindow::configUpdated);
	//}

	if (index.row() < paramModel.getParams().size()) { 
		menu.addAction (deleteAction);
		if (!paramModel.isGlobal){
			menu.addAction (addRange);
			menu.addAction (rmRange);
			if (index.column () >= paramModel.preRangeColumns && (index.column () - paramModel.preRangeColumns) % 3 != 2) {
				menu.addAction (toggleInclusivity);
			}
			auto params = paramModel.getParams();
			if ((params[index.row()].constant && true)) {
				addRange->setEnabled(false);
				rmRange->setEnabled(false);
			}
		}
	}
	menu.addAction (newParam);
	menu.addAction(dragDropTgl);

	// for test
	auto* test = new QAction("Test generateKey", &menu);
	parametersView->connect(test, &QAction::triggered, [this, index]() {
		auto allPara = getAllParams();
		generateKey(allPara, true, getRangeInfo());
		});
	menu.addAction(test);

	menu.exec (parametersView->mapToGlobal (pos));	
}

void ParameterSystem::initialize (IChimeraQtWindow* parent, std::string title, ParameterSysType type)
{
	this->setMaximumWidth(650);
	this->setMinimumWidth(450);
	QVBoxLayout* layout = new QVBoxLayout(this);
	paramSysType = type;

	parametersHeader = new QLabel (cstr (title), parent);
	parametersView = new QTableView (parent);
	layout->addWidget(parametersHeader, 0);
	layout->addWidget(parametersView, 1);

	parametersView->setModel (&paramModel);
	parametersView->show ();
	
	//parametersView->horizontalHeader ()->setFixedHeight (25);
	//parametersView->verticalHeader ()->setFixedWidth (40);
	//parametersView->verticalHeader ()->setDefaultSectionSize (22);
	parametersView->horizontalHeader ()->setStretchLastSection (true); 
	//parametersView->horizontalHeader ()->setSectionResizeMode (QHeaderView::ResizeToContents);
	parametersView->horizontalHeader ()->setSectionResizeMode (QHeaderView::Interactive);	

	/// More work to allow the column to be dragable, the more important thing is that it is annoying to change the data order in model. -ZZP 06/26/2021
	//parametersView->verticalHeader()->setSectionsMovable(true);
	//parametersView->verticalHeader()->setDragEnabled(true);
	//parametersView->verticalHeader()->setDragDropMode(QAbstractItemView::InternalMove);

	parametersView->setContextMenuPolicy (Qt::CustomContextMenu);
	parent->connect (parametersView, &QTableView::doubleClicked, parent, &IChimeraQtWindow::configUpdated);
	parent->connect(parametersView, &QTableView::customContextMenuRequested,
		[this, parent](const QPoint& pos) {
			handleContextMenu(pos);
			//parent->configUpdated(); 
		});
	parametersView->setShowGrid (true);

	if ( paramSysType == ParameterSysType::global ){
	}
	else {
		parametersView->connect (parametersView, &QTableView::doubleClicked, [this](const QModelIndex& index) {
			if (index.column() == 1) 
			{
				auto params = paramModel.getParams ();
				params[index.row ()].constant = !params[index.row ()].constant;
				paramModel.setParams (params);
			}});
	}
	parametersView->connect (&paramModel, &ParameterModel::paramsChanged, 
							 parent->scriptWin, &QtScriptWindow::updateVarNames);
	parametersView->connect(&paramModel, &ParameterModel::paramsChanged,
		[this]() {emit paramsChanged(); });
	setTableviewColumnSize ();
}

/*
 * The "normal" function, used for config and global variable systems.
 */
void ParameterSystem::handleOpenConfig(ConfigStream& configFile ){
	clearParameters( );
	paramModel.setRangeInfo (getRangeInfoFromFile (configFile));
	std::vector<parameterType> fileParams;
	try{
		fileParams = getParametersFromFile( configFile, paramModel.getRangeInfo() );
	}
	catch ( ChimeraError& )
	{/*??? Shouldn't I handle something here?*/}
	paramModel.setParams (fileParams);
	flattenScanDimensions ( );	
	setTableviewColumnSize ();
}

void ParameterSystem::setTableviewColumnSize () {
	std::vector<int> sizes(parametersView->horizontalHeader ()->count (),0);
	parametersView->horizontalHeader ()->setSectionResizeMode (QHeaderView::ResizeToContents);
	for (auto column : range(sizes.size())) {
		sizes[column] = parametersView->horizontalHeader ()->sectionSize (column);
	}
	parametersView->horizontalHeader ()->setSectionResizeMode (QHeaderView::Interactive);
	for (auto column : range (sizes.size ())) {
		parametersView->horizontalHeader ()->resizeSection (column, sizes[column]);
	}
}

ScanRangeInfo ParameterSystem::getRangeInfoFromFile (ConfigStream& configFile ){
	ScanRangeInfo rInfo;
	unsigned numRanges;
	ConfigSystem::checkDelimiterLine ( configFile, "RANGE-INFO" );
	unsigned numDimensions;
	configFile >> numDimensions;
	rInfo.setNumScanDimensions ( numDimensions );
	for ( auto dim : range( numDimensions )){
		configFile >> numRanges;
		rInfo.setNumRanges ( dim, numRanges );
		for ( auto& range : rInfo.dimensionInfo(dim) ){	
			configFile >> range.leftInclusive >> range.rightInclusive >> range.variations;
		}
	}
	return rInfo;
}

std::vector<parameterType> ParameterSystem::getParametersFromFile( ConfigStream& configFile, ScanRangeInfo rangeInfo ){
	unsigned variableNumber;
	configFile >> variableNumber;
	if ( variableNumber > 10000 ){
		auto answer = QMessageBox::question (nullptr, "Suspicious Value...",
			"variable number retrieved from file appears suspicious. The number is "
			+ qstr (variableNumber) + ". Is this accurate?");
		if ( answer == QMessageBox::No ){
			// don't try to load anything.
			variableNumber = 0;
		}
	}
	std::vector<parameterType> tempVariables;
	for ( const unsigned varInc : range( variableNumber ) ){
		tempVariables.push_back( loadParameterFromFile( configFile, rangeInfo ) );
	}
	return tempVariables;
}


void ParameterSystem::updateVariationNumber( ){
	// if no parameters, or all are constants, it will stay at 1. else, it will get set to the # of variations
	// of the first variable that it finds.
	std::vector<unsigned long> dimVariations;
	std::vector<bool> dimsSeen;
	for ( auto tempParam : paramModel.getParams() )	{
		if ( !tempParam.constant ){
			if ( dimsSeen.size( ) <= tempParam.scanDimension ){
				dimVariations.resize( tempParam.scanDimension+1, 0 );
				dimsSeen.resize( tempParam.scanDimension+1, false );
			}
			if ( dimsSeen[tempParam.scanDimension] ){
				// already seen.
				continue;
			}
			else{
				// now it's been seen, don't add it again.
				dimsSeen[tempParam.scanDimension] = true;
			}
			for ( auto range : paramModel.getRangeInfo().dimensionInfo(tempParam.scanDimension) ){
				dimVariations[ tempParam.scanDimension ] += range.variations;
			}
		}
	}
	currentVariations = 1;
	for ( auto val : dimVariations ) {
		currentVariations *= val;
	}
}

double ParameterSystem::getVariableValue ( std::string paramName ){
	if ( paramSysType != ParameterSysType::global )	{
		thrower ( "adjusting variable values in the code like this is only meant to be used with global variables!" );
	}
	bool found = false;
	for ( auto param : paramModel.getParams() ){
		if ( param.name == paramName ){
			return param.constantValue;
		}
	}
	if ( !found ){
		thrower ( "variable \"" + paramName + "\" not found in global varable control!" );
	}
	return 0;
}


void ParameterSystem::adjustVariableValue( std::string paramName, double value ){
	if ( paramSysType != ParameterSysType::global ){
		thrower ( "adjusting variable values in the code like this is only meant to be used with global variables!" );
	}
	bool found = false;
	for ( auto& param : paramModel.getParams ()){
		if ( param.name == paramName ){
			param.constantValue = value;
			found = true;
			break;
		}
	}
	if ( !found ){
		thrower ( "variable \"" + paramName + "\" not found in global varable control!" );
	}
}

parameterType ParameterSystem::loadParameterFromFile(ConfigStream& openFile, ScanRangeInfo rangeInfo ){
	parameterType tempParam;
	std::string paramName, typeText, valueString;
	openFile >> paramName >> typeText;
	std::transform( paramName.begin( ), paramName.end( ), paramName.begin( ), ::tolower );
	tempParam.name = paramName;
	if ( typeText == "Constant" ){
		tempParam.constant = true;
	}
	else if ( typeText == "Variable" ){
		tempParam.constant = false;
	}
	else{
		thrower ( "unknown parameter type option: \"" + typeText + "\" for parameter \"" + paramName 
				 + "\". Check the formatting of the configuration file." );
	}
	openFile >> tempParam.scanDimension;
	unsigned totalVariations = 0;
	for ( auto rangeInc : range( rangeInfo.numRanges(tempParam.scanDimension ) )){
		double initValue = 0, finValue = 0;
		unsigned int variations = 0;
		bool leftInclusive = 0, rightInclusive = 0;
		openFile >> initValue >> finValue;
		totalVariations += variations;
		tempParam.ranges.push_back ( { initValue, finValue } );
	}
	// shouldn't be because of 1 forcing earlier.
	if ( tempParam.ranges.size( ) == 0 ){
		// make sure it has at least one entry.
		tempParam.ranges.push_back ( { 0,0 } );
	}
	openFile >> tempParam.constantValue;
	openFile >> tempParam.parameterScope;
	return tempParam;
}

void ParameterSystem::saveParameter(ConfigStream& saveFile, parameterType parameter ){
	saveFile << "\n/*Name:*/\t\t\t" << parameter.name 
		     << "\n/*Scan-Type:*/\t\t" << ( parameter.constant ? "Constant " : "Variable " ) 
			 << "\n/*Scan-Dimension:*/\t" << parameter.scanDimension;
	for ( auto& range : parameter.ranges )	{
		saveFile << "\n/*Initial Value: */\t" << range.initialValue 
				 << "\n/*Final Value: */\t" << range.finalValue;
	}
	saveFile << "\n/*Constant Value:*/\t" << parameter.constantValue 
			 << "\n/*Scope:*/\t\t\t" << parameter.parameterScope << "\n";
}

void ParameterSystem::handleSaveConfig (ConfigStream& saveFile ){
	paramModel.checkScanDimensionConsistency ( );
	paramModel.checkVariationRangeConsistency ( );
	
	saveFile << configDelim + "\n";
	saveFile << "RANGE-INFO\n";
	saveFile << "/*# Scan Dimensions:*/\t" << paramModel.getRangeInfo ().numScanDimensions ( );
	for ( auto dimNum : range(paramModel.getRangeInfo ().numScanDimensions ( ))){
		saveFile << "\n/*Dim #" + str (dimNum + 1) + ":*/ ";
		saveFile << "\n/*Number of Ranges:*/\t" << paramModel.getRangeInfo ().numRanges ( dimNum );
		unsigned count = 0;
		auto dimInfo = paramModel.getRangeInfo ().dimensionInfo (dimNum);
		for ( auto range : dimInfo )	{
			saveFile << "\n/*Range #" + str(++count) + ":*/"
					 << "\n/*Left-Inclusive?*/\t\t" << range.leftInclusive 
					 << "\n/*Right-Inclusive?*/\t" << range.rightInclusive
					 << "\n/*# Variations: */\t\t" << range.variations;
		}
	}
	saveFile << "\n/*# Variables: */ \t\t" << getCurrentNumberOfVariables ( ); 
	for ( unsigned varInc : range(getCurrentNumberOfVariables( )))	{
		saveFile << "\n/*Variable #" + str (varInc) + "*/";
		saveParameter(saveFile, getVariableInfo( varInc ));
	}
	saveFile << "\nEND_" + configDelim + "\n";
}

void ParameterSystem::flattenScanDimensions ( ){
	while ( true ){
		unsigned maxDim = 0;
		for ( auto var : paramModel.getParams() ){
			maxDim = var.scanDimension > maxDim ? var.scanDimension : maxDim;
		}
		for ( auto dim : range ( maxDim ) ){
			bool found = false;
			for ( auto param : paramModel.getParams ()){
				if ( param.scanDimension == dim ){
					found = true;
				}
			}
			if ( !found ){
				for ( auto& param : paramModel.getParams ()){
					if ( param.scanDimension > dim ){
						param.scanDimension--;
					}
				}
				// and do the same thing again.
				continue;
			}
		}
		break;
	}
}


void ParameterSystem::setRangeInclusivity( unsigned rangeNum, unsigned dimNum, bool isLeft, bool inclusive ){
	if ( rangeNum >= paramModel.getRangeInfo ().numRanges(0) )	{
		thrower  ( "tried to set the border inclusivity of a range that does not exist!" );
	}
	( isLeft ? paramModel.getRangeInfo () ( dimNum, rangeNum ).leftInclusive 
		: paramModel.getRangeInfo () ( dimNum, rangeNum ).rightInclusive ) = inclusive;
}

void ParameterSystem::setParameterControlActive(bool active){
	controlActive = active;
}

parameterType ParameterSystem::getVariableInfo(int varNumber){
	return paramModel.getParams ()[varNumber];
}

unsigned ParameterSystem::getCurrentNumberOfVariables(){
	return paramModel.getParams ().size();
}

// takes as input parameters, but just looks at the name and usage stats. When it finds matches between the parameters,
// it takes the usage of the input and saves it as the usage of the real inputVar. 
void ParameterSystem::setUsages(std::vector<parameterType> vars){
	auto params = paramModel.getParams ();
	for ( auto inputVar : vars ){
		for ( auto& realVar : params ){
			if ( inputVar.name == realVar.name ){
				realVar.overwritten = inputVar.overwritten;
				realVar.active = inputVar.active;
				break;
			}
		}
	}
	paramModel.setParams (params);
}


void ParameterSystem::clearParameters(){
	auto params = paramModel.getParams ();
	params.clear ();
	paramModel.setParams (params);
}

std::vector<parameterType> ParameterSystem::getAllParams(){
	return paramModel.getParams ();
}

std::vector<parameterType> ParameterSystem::getAllConstants(){
	std::vector<parameterType> constants;
	for (unsigned varInc = 0; varInc < paramModel.getParams ().size(); varInc++){
		if (paramModel.getParams ()[varInc].constant){
			constants.push_back(paramModel.getParams ()[varInc]);
		}
	}
	return constants;
}

// this function returns the compliment of the parameters that "getAllConstants" returns.
std::vector<parameterType> ParameterSystem::getAllVariables(){
	// opposite of get constants.
	std::vector<parameterType> varyingParameters;
	for (unsigned varInc = 0; varInc < paramModel.getParams ().size(); varInc++){
		if (!paramModel.getParams ()[varInc].constant){
			varyingParameters.push_back(paramModel.getParams ()[varInc]);
		}
	}
	return varyingParameters;
}

void ParameterSystem::addParameter(parameterType variableToAdd){
	// make name lower case.
	std::transform(variableToAdd.name.begin(), variableToAdd.name.end(), variableToAdd.name.begin(), ::tolower);
	if (isdigit(variableToAdd.name[0]))	{
		thrower ("" + variableToAdd.name + " is an invalid name; names cannot start with numbers.");
	}
	// check for forbidden (math) characters
	if (variableToAdd.name.find_first_of(" \t\r\n()*+/-%") != std::string::npos){
		thrower ("Forbidden character in variable name! you cannot use spaces, tabs, newlines, or any of "
				"\"()*+/-%\" in a variable name.");
	}
	if (variableToAdd.name == "" ){
		return;
	}
	/// else...
	for (auto currentVar : paramModel.getParams ()){
		if (currentVar.name == variableToAdd.name){
			thrower ("A variable with the name " + variableToAdd.name + " already exists!");
		}
	}
	auto params = paramModel.getParams ();
	params.push_back (variableToAdd);
	paramModel.setParams (params);
}


ScanRangeInfo ParameterSystem::getRangeInfo ( ){
	return paramModel.getRangeInfo ();
}


std::vector<double> ParameterSystem::getRawKeyValues( std::vector<parameterType> params ){
	for ( auto variable : params){
		if ( variable.valuesVary ){
			return variable.keyValues;
		}
	}
	// no varying parameters found.
	return std::vector<double>( 1, 0 );
}

std::vector<double> ParameterSystem::getOrderedKeyValues(std::vector<parameterType> params)
{
	for (auto variable : params) {
		if (variable.valuesVary) {
			std::vector<double> keyVals(variable.keyValues.size());
			for (auto idx : range(keyVals.size())) {
				keyVals[idx] = variable.keyValues[variable.shuffleIndexReverse[idx]];
			}
			return keyVals;
		}
	}
	// no varying parameters found.
	return std::vector<double>(1, 0);
}


std::vector<parameterType> ParameterSystem::getConfigParamsFromFile( std::string configFileName ){
	std::ifstream file(configFileName);
	if (!file.is_open ()){
		thrower ("Failed to open file for config params!");
	}
	ConfigStream stream (file);
	std::vector<parameterType> configParams;
	try{
		ConfigSystem::initializeAtDelim (stream, "CONFIG_PARAMETERS");
		auto rInfo = getRangeInfoFromFile (stream);
		configParams = getParametersFromFile (stream, rInfo );
		ConfigSystem::checkDelimiterLine (stream, "END_CONFIG_PARAMETERS" );
	}
	catch ( ChimeraError& ){
		throwNested ( "Failed to get configuration parameters from the configuration file!" );
	}
	return configParams;
}


ScanRangeInfo ParameterSystem::getRangeInfoFromFile ( std::string configFileName ){
	ConfigStream stream (configFileName, true);
	ScanRangeInfo rInfo;
	try{
		ConfigSystem::initializeAtDelim (stream, "CONFIG_PARAMETERS");
		rInfo = getRangeInfoFromFile (stream);
		auto configVariables = getParametersFromFile (stream, rInfo );
		ConfigSystem::checkDelimiterLine (stream, "END_CONFIG_PARAMETERS" );
	}
	catch ( ChimeraError& ){
		throwNested ( "Failed to get configuration parameter range info from file!" );
	}
	return rInfo;
}


void ParameterSystem::generateKey( std::vector<parameterType>& parameters, bool randomizeVariationsOption,
								   ScanRangeInfo inputRangeInfo ){
	if (parameters.empty()) {
		thrower("You at least one parameter to run the experiment.");
	}
	for ( auto& variable : parameters ){
		variable.keyValues.clear( );
	}
	// find the maximum scan dimension.
	unsigned maxDim = 0;
	for ( auto variable : parameters ){
		maxDim = ( variable.scanDimension > maxDim ? variable.scanDimension : maxDim );
	}
	// each element of the vector refers to the number of variations of a given range of a given scan dimension of a 
	// given sequence element. i.e.: variationNums[dimNumber][rangeNumber]
	std::vector<std::vector<int>> variationNums( std::vector<std::vector<int>>(maxDim+1));
	std::vector<unsigned> totalSeqDimVariationsList ( std::vector<unsigned> ( maxDim+1 ) );
	// for randomizing...
	std::vector<int> variableIndexes;
	for ( auto dimInc : range( maxDim+1 ) )	{
		variationNums[dimInc].clear();
		for ( auto paramInc : range( parameters.size() ) ){
			auto& parameter = parameters[paramInc];
			// find a varying parameter in this scan dimension
			if ( parameter.scanDimension != dimInc || parameter.constant ){
				continue; 
			}
			variableIndexes.push_back( paramInc );
			if ( variationNums[dimInc].size( ) != parameter.ranges.size( ) ){
				// if its zero its just the first round of the loop. Else something has gone wrong.
				if ( variationNums[dimInc].size( ) != 0 ){
					thrower ( "Not all variables seem to have the same number of ranges for their parameters!" );
				}
				variationNums[dimInc].resize( parameter.ranges.size( ) );
			}
			totalSeqDimVariationsList[ dimInc ] = 0;
			for ( auto rangeInc : range( variationNums[dimInc].size( ) ) )	{
				variationNums[ dimInc ][ rangeInc ] = inputRangeInfo(dimInc, rangeInc).variations;
				if ( variationNums[ dimInc ][ rangeInc ] == 1 )	{
					thrower ( "You need more than one variation in every range." );
				}
				totalSeqDimVariationsList[ dimInc ] += inputRangeInfo ( dimInc, rangeInc ).variations;
			}
		}
	}
	// create a key which will be randomized and then used to randomize other things the same way.
	multiDimensionalKey<int> randomizerMultiKey( maxDim+1 );
	randomizerMultiKey.resize( totalSeqDimVariationsList );
	unsigned count = 0;
	for ( auto& keyElem : randomizerMultiKey.values ){
		keyElem = count++;
	}
	std::vector<int> MultiKeyValueCopy(randomizerMultiKey.values);
	std::vector<size_t> shuffleIndex;
	std::vector<size_t> shuffleIndexReverse;
	if ( randomizeVariationsOption ){
		std::random_device rng;
		std::mt19937 twister( rng( ) );
		std::shuffle( randomizerMultiKey.values.begin( ), randomizerMultiKey.values.end( ), twister );
	}
	std::transform(MultiKeyValueCopy.cbegin(), MultiKeyValueCopy.cend(),
		std::back_inserter(shuffleIndexReverse),
		[&](double vOriginal) {
			auto pos = std::find(randomizerMultiKey.values.begin(), randomizerMultiKey.values.end(), vOriginal);
			size_t dis = std::distance(randomizerMultiKey.values.begin(), pos);
			return dis; });
	std::transform(randomizerMultiKey.values.cbegin(), randomizerMultiKey.values.cend(), 
		std::back_inserter(shuffleIndex),
		[&](double vRandom) {
			auto pos = std::find(MultiKeyValueCopy.begin(), MultiKeyValueCopy.end(), vRandom);
			size_t dis = std::distance(MultiKeyValueCopy.begin(), pos);
			return dis; });
	if (shuffleIndex.size() == 0) {
		// means there is nothing changing in the experiment, all params are constant
		shuffleIndex = std::vector<size_t>({ 0 });
		shuffleIndexReverse = std::vector<size_t>({ 0 });
	}

	// initialize this to one so that constants always get at least one value.
	int totalSize = 1;
	for (auto variableInc : range (variableIndexes.size ())){
		auto& variable = parameters[variableIndexes[variableInc]];
		// calculate all values for a given variable
		multiDimensionalKey<double> tempKey (maxDim + 1), tempKeyRandomized (maxDim + 1);
		tempKey.resize (totalSeqDimVariationsList);
		tempKeyRandomized.resize (totalSeqDimVariationsList);
		/* Suppose you have a three dimensional scan with variation numbers 3, 2, and 3 in each dimension. Then,
		keyValueIndexes will take on the following sequence of values:
		{0,0,0} -> {1,0,0} -> {2,0,0} ->
		{0,1,0} -> {1,1,0} -> {2,1,0} ->
		{0,0,1} -> {1,0,1} -> {2,0,1} ->
		{0,1,1} -> {1,1,1} -> {2,1,1} ->
		{0,0,2} -> {1,0,2} -> {2,0,2} ->
		{0,1,2} -> {1,1,2} -> {2,1,2}.
		at which point the while loop will notice that all values turn over at the same time and leave the loop.
		*/
		std::vector<unsigned> keyValueIndexes (maxDim + 1, 0);
		while (true){
			unsigned rangeIndex = 0, varDim = variable.scanDimension, tempShrinkingIndex = keyValueIndexes[varDim],
				rangeCount = 0, rangeOffset = 0;
			// calculate which range it is and how many values have already been calculated for the variable 
			// (i.e. the rangeOffset).
			for (auto range : inputRangeInfo.dimensionInfo (varDim)){
				if (tempShrinkingIndex >= range.variations)	{
					// then should have already gone through all that range's variations
					tempShrinkingIndex -= range.variations;
					rangeOffset += range.variations;
					rangeCount++;
				}
				else{
					rangeIndex = rangeCount;
					break;
				}
			}
			auto& currRange = variable.ranges[rangeIndex];
			// calculate the parameters for the variation range
			bool lIncl = inputRangeInfo (varDim, rangeIndex).leftInclusive,
				rIncl = inputRangeInfo (varDim, rangeIndex).rightInclusive;
			int spacings = variationNums[varDim][rangeIndex] + (!lIncl && !rIncl) - (lIncl && rIncl);
			double valueRange = (currRange.finalValue - currRange.initialValue);
			double initVal = (lIncl ? currRange.initialValue : currRange.initialValue + valueRange / spacings);
			double value = valueRange * (keyValueIndexes[varDim] - rangeOffset) / spacings + initVal;
			tempKey.setValue (keyValueIndexes, value);
			bool isAtEnd = true;
			for (auto indexInc : range (keyValueIndexes.size ())){
				// if at end of cycle for this index
				if (keyValueIndexes[indexInc] == totalSeqDimVariationsList[indexInc] - 1){
					keyValueIndexes[indexInc] = 0;
				}
				else{
					keyValueIndexes[indexInc]++;
					isAtEnd = false;
					break;
				}
			}
			if (isAtEnd){
				break;
			}
		}
		for (auto keyInc : range (randomizerMultiKey.values.size ())){
			tempKeyRandomized.values[keyInc] = tempKey.values[randomizerMultiKey.values[keyInc]];
		}
		variable.keyValues = tempKeyRandomized.values;
		variable.valuesVary = true;
		variable.shuffleIndex = shuffleIndex;
		variable.shuffleIndexReverse = shuffleIndexReverse;
		totalSize = tempKeyRandomized.values.size ();
	}
	if (totalSize == 1) { // for no variation
		shuffleIndex = std::vector<size_t>(1, 0);
		shuffleIndexReverse = std::vector<size_t>(1, 0);
	}
	// now add all constant objects.
	for ( parameterType& param : parameters){
		if ( param.constant ){
			param.keyValues.clear ( );
			param.keyValues.resize( totalSize, param.constantValue );
			param.shuffleIndex = shuffleIndex;
			param.shuffleIndexReverse = shuffleIndexReverse;
			param.valuesVary = false;
		}
	}
}

/*
 * takes global params, config params, and function params, and reorganizes them to form a "parameters" object and a 
 * "constants" objects. The "parameters" object includes everything, parameters and otherwise. the "constants" object 
 * includes only parameters that don't vary. 
 */
std::vector<parameterType> ParameterSystem::combineParams( std::vector<parameterType>& configParams, 
														   std::vector<parameterType>& globalParams )
{
	std::vector<parameterType> combinedParams;
	combinedParams = configParams;
	for ( auto& sub : globalParams ){
		sub.overwritten = false;
		bool nameExists = false;
		for ( auto& var : combinedParams ){
			if ( var.name == sub.name )	{
				sub.overwritten = true;
				var.overwritten = true;
			}
		}
		if ( !sub.overwritten )	{
			combinedParams.push_back( sub );
		}
	}
	for ( auto& var : combinedParams ){
		// set the default scope for the parameters set in the normal parameter listviews. There might be a better place
		// to set this.
		if ( var.parameterScope == "" )	{
			var.parameterScope = GLOBAL_PARAMETER_SCOPE;
		}
	}
	return combinedParams;
}

unsigned ParameterSystem::getTotalVariationNumber( ){
	return currentVariations;
}
