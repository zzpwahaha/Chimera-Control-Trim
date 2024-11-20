// created by Mark O. Brown
#include "stdafx.h"

#include "RealTimeDataAnalysis/DataAnalysisControl.h"
#include "Control.h"
#include <filesystem>
#include <algorithm>
#include "PrimaryWindows/QtAndorWindow.h"
#include "ConfigurationSystems/ConfigSystem.h"
#include "RealTimeDataAnalysis/QtPlotDesignerDlg.h"
#include "RealTimeDataAnalysis/realTimePlotterInput.h"
#include "GeneralUtilityFunctions/range.h"
#include <PrimaryWindows/QtMainWindow.h>
#include <numeric>
#include <boost/tuple/tuple.hpp>
#include <boost/lexical_cast.hpp>
#include <map>
#include <qheaderview.h>
#include <qmenu.h>

DataAnalysisControl::DataAnalysisControl (IChimeraQtWindow* parent) 
	: IChimeraSystem(parent)
{
	reloadTinyPlotInfo();
	currentSettings.grids.resize( 1 );
}

void DataAnalysisControl::handleContextMenu (const QPoint& pos) {
	QTableWidgetItem* item = plotListview->itemAt (pos);
	QMenu menu;
	menu.setStyleSheet (chimeraStyleSheets::stdStyleSheet ());
	auto* deleteAction = new QAction ("Delete Plot", plotListview);
	plotListview->connect (deleteAction, &QAction::triggered, [this, item]() {
		auto name = plotListview->item (item->row (), 0)->text();
		tinyPlotInfo selectedInfo;
		unsigned selectedIndex = 0;
		for (auto& pltInfo : allTinyPlots) {
			if (pltInfo.name == str(name)) {
				selectedInfo = pltInfo;
				break;
			}
			selectedIndex++;
		}
		auto answer = QMessageBox::question ( plotListview, qstr ("Delete Plot?"),
											  qstr("Delete Plot " + selectedInfo.name + "?"));
		if (answer == QMessageBox::Yes){
			int result = DeleteFile (cstr (PLOT_FILES_SAVE_LOCATION + "\\" + selectedInfo.name + "." 
										   + PLOTTING_EXTENSION ));
			if (!result){
				errBox ("Failed to delete script file! Error code: " + str (GetLastError ()));
				return;
			}
			allTinyPlots.erase (allTinyPlots.begin () + selectedIndex);
			reloadListView ();
		}});
	auto* detailsAction = new QAction ("View Plot Details", plotListview);
	plotListview->connect (detailsAction, &QAction::triggered, [this, item]() {
		try {
			auto name = plotListview->item (item->row (), 0)->text ();
			tinyPlotInfo selectedInfo;
			unsigned selectedIndex = 0;
			for (auto& pltInfo : allTinyPlots) {
				if (pltInfo.name == str (name)) {
					selectedInfo = pltInfo;
					break;
				}
				selectedIndex++;
			}
			infoBox (PlottingInfo::getAllSettingsStringFromFile (
				PLOT_FILES_SAVE_LOCATION + "\\" + selectedInfo.name + "." + PLOTTING_EXTENSION));
		}
		catch (ChimeraError & err) {
			errBox (err.trace ());
		}});
	menu.addAction (detailsAction);
	auto* editAction = new QAction ("Edit Plot", plotListview);
	plotListview->connect (editAction, &QAction::triggered, [this, item]() {
		auto name = plotListview->item(item->row(), 0)->text();
		tinyPlotInfo selectedInfo;
		unsigned selectedIndex = 0;
		for (auto& pltInfo : allTinyPlots) {
			if (pltInfo.name == str(name)) {
				selectedInfo = pltInfo;
				break;
			}
			selectedIndex++;
		}
		try {
			QtPlotDesignerDlg* dialog = new QtPlotDesignerDlg(PLOT_FILES_SAVE_LOCATION + "\\" + selectedInfo.name + "." + PLOTTING_EXTENSION/*, unofficialPicsPerRep*/);
			dialog->setStyleSheet(chimeraStyleSheets::stdStyleSheet());
			dialog->setAttribute(Qt::WA_DeleteOnClose);
			dialog->exec();
		}
		catch (ChimeraError& err) {
			errBox(err.trace());
		}

		//allTinyPlots.push_back (newPlot);
		reloadListView(); });
	menu.addAction (editAction);

	auto* newPerson = new QAction ("New Plot", plotListview);
	plotListview->connect (newPerson, &QAction::triggered, [this]() {
		QtPlotDesignerDlg* dialog = new QtPlotDesignerDlg(unofficialPicsPerRep);
		dialog->setStyleSheet(chimeraStyleSheets::stdStyleSheet());
		dialog->setAttribute(Qt::WA_DeleteOnClose);
		dialog->exec();
		//allTinyPlots.push_back (newPlot);
		reloadTinyPlotInfo();
		reloadListView();
		});
	if (item) { menu.addAction (deleteAction); }
	menu.addAction (newPerson);

	auto* refresh = new QAction("Refresh Plot List", plotListview);
	plotListview->connect(refresh, &QAction::triggered, [this]() {
		reloadTinyPlotInfo();
		reloadListView();
		});
	if (item) { menu.addAction(deleteAction); }
	menu.addAction(refresh);

	menu.exec (plotListview->mapToGlobal (pos));
}

void DataAnalysisControl::initialize( IChimeraQtWindow* parent ){
	QVBoxLayout* layout = new QVBoxLayout(this);
	layout->setContentsMargins(0, 0, 0, 0);

	header = new QLabel ("DATA ANALYSIS", parent);
	layout->addWidget(header);

	QHBoxLayout* layout1 = new QHBoxLayout(this);
	layout1->setContentsMargins(0, 0, 0, 0);
	currentDataSetNumberText = new QLabel ("Data Set #:", parent);
	currentDataSetNumberDisp = new QLabel ("?", parent);
	currentDataSetNumberText->setStyleSheet("QLabel { font: 24pt; };");
	currentDataSetNumberDisp->setStyleSheet("QLabel { font: bold 24pt; };");
	layout1->addWidget(currentDataSetNumberText, 0);
	layout1->addWidget(currentDataSetNumberDisp, 1);

	QHBoxLayout* layout2 = new QHBoxLayout(this);
	layout2->setContentsMargins(0, 0, 0, 0);
	QLabel* gridSelectorLabel = new QLabel("Grid:", parent);
	gridSelector = new CQComboBox (parent);
	parent->connect (gridSelector, qOverload<int>(&QComboBox::currentIndexChanged), 
		[this, parent]() {
			try{
				handleAtomGridCombo ();
			}
			catch (ChimeraError& err){
				parent->reportErr (err.qtrace ());
			}
		});
	gridSelector->addItem ("0");
	gridSelector->setCurrentIndex( 0 );	

	newGrid = new CQPushButton("New", parent);
	parent->connect(newGrid, &QPushButton::released, [this, parent]() {
		try {
			currentSettings.grids.emplace_back();
			reloadGridCombo();
		}
		catch (ChimeraError& err) {
			parent->reportErr(err.qtrace());
		}
		});

	deleteGrid = new CQPushButton ("Del", parent);
	parent->connect (deleteGrid, &QPushButton::released, [this, parent]() {
			try{
				handleDeleteGrid (gridSelector->currentIndex());
			}
			catch (ChimeraError& err){
				parent->reportErr (err.qtrace ());
			}
		});

	refreshGrid = new CQPushButton("Refr.", parent);
	parent->connect(refreshGrid, &QPushButton::released, [this, parent]() {
		try {
			int sel = gridSelector->currentIndex();
			std::vector<std::string> gridNames = getGridFileNames();
			auto& grids = currentSettings.grids;
			for (auto& gridName : gridNames) {
				if (std::find_if(grids.begin(), grids.end(), [this, gridName](auto& g) {
					return g.useFile && (g.fileName == gridName);
					}) == grids.end()) { // means this grid file is not in the current setting grids
					grids.emplace_back();
					grids.back().useFile = true;
					grids.back().fileName = gridName;
					grids.back().loadGridFile(grids.back());
				}
			}
			reloadGridCombo();
			gridSelector->setCurrentIndex(sel);

		}
		catch (ChimeraError& err) {
			parent->reportErr(err.qtrace());
		}
		});


	displayGridBtn = new CQCheckBox("Disp. Grid?", parent);
	parent->connect(displayGridBtn, &QCheckBox::released, [this, parent]() {
		updateSettings();
		if (displayGridBtn->isChecked()) {
			int sel = gridSelector->currentIndex();
			parent->andorWin->displayAnalysisGrid(currentSettings.grids[sel]);
		}
		else {
			parent->andorWin->removeAnalysisGrid();
		}
		});

	layout2->addWidget(gridSelectorLabel, 0);
	layout2->addWidget(gridSelector, 1);
	layout2->addWidget(newGrid, 0);
	layout2->addWidget(deleteGrid, 0);
	layout2->addWidget(refreshGrid, 0);
	layout2->addWidget(displayGridBtn, 0);

	QGridLayout* layout3 = new QGridLayout(this);
	layout3->setContentsMargins(0, 0, 0, 0);
	layout3->addWidget(new QLabel("Origin Coord.", this), 0, 1);
	layout3->addWidget(new QLabel("Spacing", this), 0, 2);
	layout3->addWidget(new QLabel("#", this), 0, 3);
	layout3->addWidget(new QLabel("Bin Incl.", this), 0, 4);

	layout3->addWidget(new QLabel("X", this), 1, 0);
	originEditX = new CQLineEdit("0", parent);
	gridSpacingX = new CQLineEdit("0", parent);
	gridNumberX = new CQLineEdit("0", parent);
	includePixelX = new CQLineEdit("0", parent);
	layout3->addWidget(originEditX, 1, 1);
	layout3->addWidget(gridSpacingX, 1, 2);
	layout3->addWidget(gridNumberX, 1, 3);
	layout3->addWidget(includePixelX, 1, 4);

	layout3->addWidget(new QLabel("Y", this), 2, 0);
	originEditY = new CQLineEdit("0", parent);
	gridSpacingY = new CQLineEdit("0", parent);
	gridNumberY = new CQLineEdit("0", parent);
	includePixelY = new CQLineEdit("0", parent);
	layout3->addWidget(originEditY, 2, 1);
	layout3->addWidget(gridSpacingY, 2, 2);
	layout3->addWidget(gridNumberY, 2, 3);
	layout3->addWidget(includePixelY, 2, 4);

	for (auto* b : { originEditX,originEditY,gridSpacingX,gridSpacingY,gridNumberX,gridNumberY,includePixelX,includePixelY }) {
		parent->connect(b, &QLineEdit::returnPressed, this, [this]() {saveGridParams(); });
		//b->setStyleSheet("QLineEdit[readOnly=\"true\"] {color: #808080; background-color: #F0F0F0;}");
	}

	/// PLOTTING FREQUENCY CONTROLS
	//QHBoxLayout* layout4 = new QHBoxLayout(this);
	//layout4->setContentsMargins(0, 0, 0, 0);

	QHBoxLayout* layout5 = new QHBoxLayout(this);
	layout5->setContentsMargins(0, 0, 0, 0);
	autoThresholdAnalysisButton = new CQCheckBox ("Auto Threshold Analysis", parent);
	autoThresholdAnalysisButton->setToolTip ("At the end of an experiment, run some python code which will fit the "
		"data and determine good thresholds which can be outputted to a file to "
		"keep the thresholds used by the real-time analysis up-to-date.");

	
	autoBumpAnalysis = new CQCheckBox ("Auto Bump Analysis", parent);
	autoBumpAnalysis->setToolTip ("At the end of the experiment, run some python code which will do standard data "
		"analysis on the resulting data set and fit a bump to it. The bump center value will be written to a file.");
	doBumpAnalysis = new CQPushButton ("Analyze Now", parent);

	parent->connect (doBumpAnalysis, &QPushButton::released, [parent]() {
		parent->andorWin->handleBumpAnalysis (parent->mainWin->getProfileSettings());
		});
	bumpEditParam = new CQLineEdit (parent);
	layout5->addWidget(autoThresholdAnalysisButton, 0);
	layout5->addWidget(autoBumpAnalysis, 0);
	layout5->addWidget(doBumpAnalysis, 0);
	layout5->addWidget(bumpEditParam, 1);


	/// Initialize the listview
	plotListview = new QTableWidget (parent);
	QStringList labels;
	labels << " Name " << " Grid # " << " Active ";
	plotListview->setContextMenuPolicy (Qt::CustomContextMenu);
	parent->connect (plotListview, &QTableWidget::customContextMenuRequested,
					 [this](const QPoint& pos) { handleContextMenu (pos); });
	plotListview->setColumnCount (labels.size ());
	plotListview->setHorizontalHeaderLabels (labels);
	plotListview->horizontalHeader ()->setFixedHeight (25);
	plotListview->horizontalHeader ()->setSectionResizeMode (QHeaderView::Stretch);
	plotListview->resizeColumnsToContents ();
	plotListview->verticalHeader ()->setSectionResizeMode (QHeaderView::Fixed);
	plotListview->verticalHeader ()->setDefaultSectionSize (22);
	plotListview->verticalHeader ()->setFixedWidth (40);
	parent->connect (plotListview, &QTableWidget::cellChanged, [this, parent](int row, int col) {
			if (col == 1) {
				auto* item = plotListview->item (row, 0);
				auto itemName = str(item->text());
				unsigned counter = 0;
				for (auto& pltInfo : allTinyPlots) {
					if (pltInfo.name == itemName) {
						break;
					}
					counter++;
				}
				item = plotListview->item(row, col);
				try {
					allTinyPlots[counter].whichGrid = boost::lexical_cast<unsigned>( cstr (item->text ()));
				}
				catch (ChimeraError&) {}
			}
		});
	parent->connect (plotListview, &QTableWidget::cellDoubleClicked, 
		[this, parent](int clRow, int clCol) {
			parent->configUpdated ();
			if (clCol == 2) {
				auto* item = plotListview->item (clRow, 0);
				unsigned counter = 0;
				for (auto& pltInfo : allTinyPlots) {
					if (pltInfo.name == str(item->text ())) {
						break;
					}
					counter++;
				}
				tinyPlotInfo& info = allTinyPlots[counter];
				info.isActive = !info.isActive;
				auto* newItem = new QTableWidgetItem (info.isActive ? "YES" : "NO");
				newItem->setFlags (newItem->flags () & ~Qt::ItemIsEditable);
				plotListview->setItem (clRow, clCol, newItem);
			}
		});
	reloadListView();

	layout->addLayout(layout1);
	layout->addLayout(layout2);
	layout->addLayout(layout3);
	//layout->addLayout(layout4);
	layout->addLayout(layout5);
	layout->addWidget(plotListview);

	emit refreshGrid->released();
}

void DataAnalysisControl::handleDeleteGrid(int sel){
	if (currentSettings.grids.size() == 1 ){
		thrower ( "ERROR: You are not allowed to delete the last grid for data analysis!" );
	}
	if (sel == -1) {
		return;
	}
	if (sel >= currentSettings.grids.size()) {
		thrower("Error: The selected grid number is larger than the internal grid data. A low level bug.");
	}
	currentSettings.grids.erase(currentSettings.grids.begin( ) + sel);
	reloadGridCombo();
	gridSelector->setCurrentIndex( 0 );
	loadGridParams(currentSettings.grids[0] );
}

void DataAnalysisControl::updateDisplays (analysisSettings settings) {
	autoThresholdAnalysisButton->setChecked (settings.autoThresholdAnalysisOption);
	displayGridBtn->setChecked (settings.displayGridOption);
	autoBumpAnalysis->setChecked (settings.autoBumpOption);
	bumpEditParam->setText (qstr (settings.bumpParam));
	unsigned counter = 0;
	for (auto& pltInfo : allTinyPlots) {
		unsigned activeCounter = -1;
		pltInfo.isActive = false;
		for (auto activePlt : settings.activePlotNames) {
			activeCounter++;
			if (activePlt == pltInfo.name) {
				pltInfo.isActive = true;
				pltInfo.whichGrid = settings.whichGrids[activeCounter];
				break;
			}
		}
		counter++;
	}
	// load the grid parameters for that selection.
	loadGridParams (currentSettings.grids[0]);
	reloadGridCombo ();
	gridSelector->setCurrentIndex (0);
	reloadListView ();
}

void DataAnalysisControl::setAnalysisSettings (analysisSettings settings) {
	currentSettings = settings;
	updateDisplays (currentSettings);
}

void DataAnalysisControl::handleOpenConfig( ConfigStream& file ){
	analysisSettings settings = getAnalysisSettingsFromFile (file);
	setAnalysisSettings (settings);
}

analysisSettings DataAnalysisControl::getAnalysisSettingsFromFile (ConfigStream& file) {
	analysisSettings settings;
	unsigned numGrids=1;
	file >> settings.autoThresholdAnalysisOption;
	file >> numGrids;
	if (numGrids <= 0) {
		numGrids = 1;
	}
	settings.grids.resize (numGrids);
	for (auto& grid : settings.grids) {
		file >> grid.gridOrigin.row >> grid.gridOrigin.column >> grid.width >> grid.height
			>> grid.pixelSpacingX >> grid.pixelSpacingY >> grid.includedPixelX >> grid.includedPixelY >> grid.useFile >> grid.fileName;
	}
	// load the grid parameters for that selection.
	ConfigSystem::checkDelimiterLine (file, "BEGIN_ACTIVE_PLOTS");
	unsigned numPlots = 0;
	file >> numPlots;
	file.get ();
	for (auto pltInc : range (numPlots)) {
		std::string tmp = file.getline ();
		settings.activePlotNames.push_back (tmp);
		unsigned which;
		file >> which;
		file.get ();
		settings.whichGrids.push_back (which);
	}
	ConfigSystem::checkDelimiterLine (file, "END_ACTIVE_PLOTS");
	file.get ();
	file >> settings.displayGridOption;

	file >> settings.autoBumpOption;
	file >> settings.bumpParam;
	return settings;
}

std::pair<bool, std::string> DataAnalysisControl::getBumpAnalysisOptions (ConfigStream& file) {
	auto settings = getAnalysisSettingsFromFile (file);
	return { settings.autoBumpOption, settings.bumpParam };
}

void DataAnalysisControl::updateSettings () {
	saveGridParams ();
	currentSettings.autoThresholdAnalysisOption = autoThresholdAnalysisButton->isChecked ();
	currentSettings.displayGridOption = displayGridBtn->isChecked ();
	currentSettings.autoBumpOption = autoBumpAnalysis->isChecked ();
	currentSettings.bumpParam = str (bumpEditParam->text ());
	currentSettings.activePlotNames = getActivePlotList ();
}

void DataAnalysisControl::handleSaveConfig( ConfigStream& file ){
	updateSettings ();
	file << "DATA_ANALYSIS\n";
	file << "/*Auto-Threshold Analysis?*/\t" << currentSettings.autoThresholdAnalysisOption;
	file << "\n/*Number of Analysis Grids: */\t" << currentSettings.grids.size ();
	unsigned count = 0;
	for ( auto& grid : currentSettings.grids ){
		file << "\n/*Grid #" + str(++count) << ":*/ "
			<< "\n/*Grid Origin X(Bottom-Left Corner Column):*/\t\t" << grid.gridOrigin.column
			<< "\n/*Grid Origin Y(Bottom-Left Corner Row):*/\t\t" << grid.gridOrigin.row
			<< "\n/*Grid Width:*/\t\t\t\t\t" << grid.width
			<< "\n/*Grid Height:*/\t\t\t\t" << grid.height
			<< "\n/*Pixel Spacing X:*/\t\t\t" << grid.pixelSpacingX
			<< "\n/*Pixel Spacing Y:*/\t\t\t" << grid.pixelSpacingY
			<< "\n/*Include Pixels X:*/\t\t\t" << grid.includedPixelX
			<< "\n/*Include Pixels Y:*/\t\t\t" << grid.includedPixelY
			<< "\n/*Use External File:*/\t\t\t" << grid.useFile
			<< "\n/*Grid File Name:*/\t\t\t\t" << grid.fileName;
	}
	file << "\nBEGIN_ACTIVE_PLOTS\n";
	unsigned activeCount = 0;
	for ( auto miniPlot : allTinyPlots ){
		if ( miniPlot.isActive ){
			activeCount++;
		}
	}
	file << "/*Number of Active Plots:*/ " << activeCount;
	count = 0;
	for ( auto miniPlot : allTinyPlots ){
		if ( miniPlot.isActive ){
			file << "\n/*Active Plot #" + str (++count) + "*/";
			file << "\n/*Plot Name:*/ " << miniPlot.name;
			file << "\n/*Which Grid:*/ " << miniPlot.whichGrid;
		}
	}
	file << "\nEND_ACTIVE_PLOTS\n";
	file << "/*Display Grid?*/ " << currentSettings.displayGridOption << "\n";
	file << "/*Auto Bump Analysis?*/ " << currentSettings.autoBumpOption << "\n";
	file << "/*Bump Param?*/ " << currentSettings.bumpParam << "\n";
	file << "END_DATA_ANALYSIS\n"; 
}



void DataAnalysisControl::fillPlotThreadInput(realTimePlotterInput* input){
	std::vector<tinyPlotInfo> usedPlots;
	input->plotInfo.clear();
	for (auto activeName : currentlyRunningSettings.activePlotNames) {
		for (auto plt : allTinyPlots ){
			if (plt.name == activeName) {
				input->plotInfo.push_back (plt);
				break;
			}
		}
	}
	input->grids = currentlyRunningSettings.grids;
	for (auto& grid : input->grids) {
		grid.loadGridFile(grid); // make sure all grids in the real time analysis is loaded
	}
	// as I fill the input, also check this, which is necessary info for plotting.
	input->needsCounts = false;
	for (auto plt : input->plotInfo){
		PlottingInfo info(PLOT_FILES_SAVE_LOCATION + "\\" + plt.name + "." + PLOTTING_EXTENSION);
		if (info.getPlotType() != "Atoms"){
			input->needsCounts = true;
		}
	}
}

void DataAnalysisControl::handleAtomGridCombo( ){
	int sel = gridSelector->currentIndex( );
	if ( sel == -1 ){
		return;
	}
	else if (sel >= currentSettings.grids.size()){
		thrower ( "ERROR: Bad value for atom grid combobox selection???  (A low level bug, this shouldn't happen)" );
	}
	// load the grid parameters for that selection.
	loadGridParams(currentSettings.grids[sel] );
}

void DataAnalysisControl::reloadGridCombo( ){
	gridSelector->clear( );
	unsigned count = 0;
	for ( const auto& grid : currentSettings.grids ){
		std::string txt( str( count++ ) );
		gridSelector->addItem(qstr(txt) + (grid.useFile ? ": " + qstr(grid.fileName) : ""));
	}
}

void DataAnalysisControl::loadGridParams( atomGrid& grid ){
	if (!gridSpacingX || !gridSpacingY || !gridNumberX || !gridNumberY) {
		return;
	}
	originEditX->setText(qstr(grid.gridOrigin.column));
	originEditY->setText(qstr(grid.gridOrigin.row));
	std::string txt = str(grid.pixelSpacingX);
	gridSpacingX->setText(qstr(txt));
	txt = str(grid.pixelSpacingY);
	gridSpacingY->setText(qstr(txt));
	txt = str(grid.width);
	gridNumberX->setText(qstr(txt));
	txt = str(grid.height);
	gridNumberY->setText(qstr(txt));
	txt = str(grid.includedPixelX);
	includePixelX->setText(qstr(txt));
	txt = str(grid.includedPixelX);
	includePixelY->setText(qstr(txt));
	if (grid.useFile) {
		gridSelector->setToolTip(qstr(grid.fileName));
		for (auto* b : { originEditX,originEditY,gridSpacingX,gridSpacingY,gridNumberX,gridNumberY,includePixelX,includePixelY }) {
			b->setEnabled(false);
			b->setStyleSheet("QLineEdit { background: rgb(204, 204, 204); }");
		}
		atomGrid::loadGridFile(grid);
	}
	else {
		for (auto* b : { originEditX,originEditY,gridSpacingX,gridSpacingY,gridNumberX,gridNumberY,includePixelX,includePixelY }) {
			b->setEnabled(true);
			b->setStyleSheet("QLineEdit { background: rgb(255, 255, 255); }");
		}
	}
	if (displayGridBtn->isChecked()) {
		parentWin->andorWin->removeAnalysisGrid();
		parentWin->andorWin->displayAnalysisGrid(grid);
	}
}

void DataAnalysisControl::saveGridParams( ){
	if (!gridSpacingX || !gridSpacingY || !gridNumberX || !gridNumberY) {
		return;
	}
	int sel = gridSelector->currentIndex();
	if (sel == -1) {
		return;
	}
	if (sel >= currentSettings.grids.size()) {
		thrower("Error: The selected grid number is larger than the internal grid data. A low level bug.");
	}
	unsigned row = 0, col = 0;
	try{
		row = boost::lexical_cast<unsigned long> (str (originEditY->text ()));
		col = boost::lexical_cast<unsigned long> (str (originEditX->text ()));
		currentSettings.grids[sel].gridOrigin = { row, col };
		currentSettings.grids[sel].pixelSpacingX = boost::lexical_cast<long>( str(gridSpacingX->text()) );
		currentSettings.grids[sel].pixelSpacingY = boost::lexical_cast<long>(str(gridSpacingY->text()));
		currentSettings.grids[sel].height = boost::lexical_cast<long>( str( gridNumberY->text() ) );
		currentSettings.grids[sel].width = boost::lexical_cast<long>( str(gridNumberX->text()) );
		currentSettings.grids[sel].includedPixelX = boost::lexical_cast<long>(str(includePixelX->text()));
		currentSettings.grids[sel].includedPixelY = boost::lexical_cast<long>(str(includePixelY->text()));
	}
	catch ( boost::bad_lexical_cast&){
		throwNested ( "ERROR: failed to convert grid parameters to longs while saving grid data!" );
	}
}

std::vector<std::string> DataAnalysisControl::getActivePlotList(){
	std::vector<std::string> list;
	for ( auto plot : allTinyPlots ){
		if ( plot.isActive ) {
			list.push_back( plot.name );
		}
	}
	return list;
}

void DataAnalysisControl::updateDataSetNumberEdit( int number ){
	if ( number > 0 ){
		currentDataSetNumberDisp->setText( cstr( number ) );
	}
	else{
		currentDataSetNumberDisp->setText ( "None" );
	}
}

void DataAnalysisControl::reloadListView(){
	plotListview->setRowCount (0);
	for (auto item : allTinyPlots){
		if (item.numPics != unofficialPicsPerRep) {
			continue;
		}		
		int row = plotListview->rowCount ();
		plotListview->insertRow (row);
		plotListview->setItem (row, 0, new QTableWidgetItem (item.name.c_str()));
		plotListview->setItem (row, 1, new QTableWidgetItem (cstr(item.whichGrid)));
		auto item3 = new QTableWidgetItem (item.isActive ? "YES" : "NO");
		item3->setFlags (item3->flags () & ~Qt::ItemIsEditable);
		plotListview->setItem (row, 2, item3);
	}
}

void DataAnalysisControl::reloadTinyPlotInfo()
{
	allTinyPlots.clear();
	std::vector<std::string> names = ConfigSystem::searchForFiles(PLOT_FILES_SAVE_LOCATION, str("*.") + PLOTTING_EXTENSION);
	for (auto name : names) {
		PlottingInfo totalInfo(PLOT_FILES_SAVE_LOCATION + "\\" + name + "." + PLOTTING_EXTENSION);
		tinyPlotInfo info;
		info.name = name;
		info.isHist = (totalInfo.getPlotType() == "Pixel_Count_Histograms");
		info.numPics = totalInfo.getPicNumber();
		allTinyPlots.push_back(info);
	}
}

void DataAnalysisControl::updateUnofficialPicsPerRep (unsigned ppr, bool continuousMode) {
	unofficialPicsPerRep = ppr;
	if (!continuousMode) {
		plotListview->setEnabled(true);
		reloadListView();
	}
	else {
		plotListview->setDisabled(true);
	}
}

analysisSettings DataAnalysisControl::getConfigSettings () {
	updateSettings ();
	return currentSettings;
}

analysisSettings DataAnalysisControl::getRunningSettings() {
	return currentlyRunningSettings;
}

std::vector<std::string> DataAnalysisControl::getGridFileNames()
{
	std::vector<std::string> gridDirs;
	std::string path(PLOT_FILES_SAVE_LOCATION);
	std::string ext(atomGrid::GRID_FILE_EXTENSION);
	try {
		for (auto& p : std::filesystem::recursive_directory_iterator(path))
		{
			if (p.path().extension() == ext) {
				gridDirs.push_back(p.path().stem().string());
			}
		}
	}
	catch (std::filesystem::filesystem_error const& ex) {
		throwNested(ex.what());
	}

	return gridDirs;
}

void DataAnalysisControl::setRunningSettings (analysisSettings options) {
	currentlyRunningSettings = options;
}