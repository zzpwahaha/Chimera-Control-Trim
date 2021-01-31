#include "stdafx.h"
#include "ftd2xx.h"
#include "DirectDigitalSynthesis/DdsSystem.h"
#include "PrimaryWindows/QtAuxiliaryWindow.h"
#include "GeneralObjects/multiDimensionalKey.h"
#include <ParameterSystem/Expression.h>
#include <boost/lexical_cast.hpp>
#include <qheaderview.h>
#include <qmenu.h>
#include <PrimaryWindows/QtMainWindow.h>

DdsSystem::DdsSystem (IChimeraQtWindow* parent, bool ftSafemode) : core( ftSafemode )
	, IChimeraSystem (parent) { }

void DdsSystem::handleContextMenu (const QPoint& pos)
{
	QTableWidgetItem* item = rampListview->itemAt (pos);
	QMenu menu;
	menu.setStyleSheet (chimeraStyleSheets::stdStyleSheet ());
	auto* deleteAction = new QAction ("Delete This Item", rampListview);
	rampListview->connect (deleteAction, &QAction::triggered, [this, item]() {
		refreshCurrentRamps ();
		currentRamps.erase(currentRamps.begin()+item->row());
		redrawListview ();
		});
	auto* newPerson = new QAction ("New Item", rampListview);
	rampListview->connect (newPerson, &QAction::triggered,
		[this]() {
			refreshCurrentRamps ();
			currentRamps.push_back (ddsIndvRampListInfo ());
			redrawListview ();
		});
	if (item) { menu.addAction (deleteAction); }
	menu.addAction (newPerson);
	menu.exec (rampListview->mapToGlobal (pos));
}

void DdsSystem::initialize ( IChimeraQtWindow* parent, std::string title ){
	QVBoxLayout* layout = new QVBoxLayout(this);
	this->setMaximumWidth(900);

	ddsHeader = new QLabel (cstr (title), parent);
	layout->addWidget(ddsHeader, 0);
	QHBoxLayout* layout1 = new QHBoxLayout();
	layout1->setContentsMargins(0, 0, 0, 0);
	programNowButton = new QPushButton ("Program Now", parent);
	layout1->addWidget(programNowButton, 0);
	parent->connect (programNowButton, &QPushButton::released, [this, parent]() {
		try	{
			programNow (parent->auxWin->getUsableConstants ());
		}
		catch (ChimeraError& err) {
			parent->reportErr (err.qtrace ());
		}
	});
	controlCheck = new CQCheckBox ("Control?", parent);
	layout1->addWidget(controlCheck, 0);
	layout->addLayout(layout1);
	rampListview = new QTableWidget (parent);
	layout->addWidget(rampListview);

	rampListview->horizontalHeader()->setDefaultSectionSize(80);
	rampListview->setColumnWidth (0, 60);
	rampListview->setColumnWidth (1, 60);
	rampListview->setColumnWidth (2, 60);
	rampListview->setColumnWidth (3, 60);
	rampListview->setColumnWidth (4, 60);
	rampListview->setColumnWidth (5, 60);
	rampListview->setColumnWidth (6, 120);
	//rampListview->verticalHeader()->setDefaultSectionSize(80);

	rampListview->setContextMenuPolicy (Qt::CustomContextMenu);
	parent->connect (rampListview, &QTableWidget::customContextMenuRequested,
		[this](const QPoint& pos) {handleContextMenu (pos); });
	rampListview->setShowGrid (true);

	QStringList labels;
	labels << "Index" << "Channel" << "Freq 1" << "Amp 1" << "Freq 2" << "Amp 2" << "Time";
	rampListview->setColumnCount (labels.size());
	rampListview->setHorizontalHeaderLabels (labels);
}

void DdsSystem::redrawListview ( ){
	rampListview->setRowCount (0);
	for (auto rampInc : range (currentRamps.size ()))	{
		rampListview->insertRow (rampListview->rowCount ());
		auto rowN = rampListview->rowCount ()-1;
		auto& ramp = currentRamps[rampInc];
		rampListview->setItem (rowN, 0, new QTableWidgetItem (cstr (ramp.index)));
		rampListview->setItem (rowN, 1, new QTableWidgetItem (cstr (ramp.channel)));
		rampListview->setItem (rowN, 2, new QTableWidgetItem (cstr (ramp.freq1.expressionStr)));
		rampListview->setItem (rowN, 3, new QTableWidgetItem (cstr (ramp.amp1.expressionStr)));
		rampListview->setItem (rowN, 4, new QTableWidgetItem (cstr (ramp.freq2.expressionStr)));
		rampListview->setItem (rowN, 5, new QTableWidgetItem (cstr (ramp.amp2.expressionStr)));
		rampListview->setItem (rowN, 6, new QTableWidgetItem (cstr (ramp.rampTime.expressionStr)));
	}
}

void DdsSystem::programNow ( std::vector<parameterType>& constants ){
	try{
		refreshCurrentRamps ();
		core.manualLoadExpRampList (currentRamps);
		core.evaluateDdsInfo ( constants );
		core.generateFullExpInfo ( 1 );
		core.programVariation ( 0, constants, nullptr);
		emit notification ("Finished Programming DDS System!\n", 0);
	}
	catch ( ChimeraError& ){
		throwNested ( "Error seen while programming DDS system via Program Now Button." );
	}
}


void DdsSystem::handleSaveConfig (ConfigStream& file ){
	refreshCurrentRamps ();
	file << getDelim() << "\n";
	file << "/*Control?*/ " << controlCheck->isChecked () << "\n";
	core.writeRampListToConfig ( currentRamps, file );
	file << "\nEND_" + getDelim ( ) << "\n";
}


void DdsSystem::handleOpenConfig ( ConfigStream& file ){
	auto res = core.getSettingsFromConfig (file);
	currentRamps = res.ramplist;
	controlCheck->setChecked (res.control);
	redrawListview ( );
}


std::string DdsSystem::getSystemInfo ( ){
	return core.getSystemInfo();
}


std::string DdsSystem::getDelim ( ){
	return core.configDelim;
}

DdsCore& DdsSystem::getCore ( ){
	return core;
}

void DdsSystem::refreshCurrentRamps () {
	currentRamps.resize (rampListview->rowCount ());
	for (auto rowI : range(rampListview->rowCount ())) {
		try {
			currentRamps[rowI].index = boost::lexical_cast<int>(cstr(rampListview->item (rowI, 0)->text ()));
			currentRamps[rowI].channel = boost::lexical_cast<int>(cstr (rampListview->item (rowI, 1)->text ()));
			currentRamps[rowI].freq1 = str (rampListview->item (rowI, 2)->text ());
			currentRamps[rowI].amp1 = str(rampListview->item (rowI, 3)->text ());
			currentRamps[rowI].freq2 = str(rampListview->item (rowI, 4)->text ());
			currentRamps[rowI].amp2 = str(rampListview->item (rowI, 5)->text ());
			currentRamps[rowI].rampTime = str(rampListview->item (rowI, 6)->text ());
		}
		catch (ChimeraError &) {
			throwNested ("Failed to convert dds table data to ramp structure!");
		}
	}
}