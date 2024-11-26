#include "stdafx.h"
#include "QtAnalysisWindow.h"
#include <PrimaryWindows/QtScriptWindow.h>
#include <PrimaryWindows/QtAndorWindow.h>
#include <PrimaryWindows/QtAuxiliaryWindow.h>
#include <PrimaryWindows/QtMainWindow.h>
#include <ExperimentMonitoringAndStatus/colorbox.h>


QtAnalysisWindow::QtAnalysisWindow(QWidget* parent) 
	: IChimeraQtWindow(parent)
	, MOTAnalySys(this)
	, SeqPlotter(this)
	, staticDac(this)
	, staticDds(this)
	, elliptec(this)
{
	setWindowTitle("Analysis Window");
}


std::string QtAnalysisWindow::getSystemStatusString()
{
	std::string msg;
	msg += "Static AO System:\n";
	if (!STATICAO_SAFEMODE) {
		msg += str("\tStatic AO System is Active at ") + STATICAO_IPADDRESS + ", at port " + str(STATICAO_IPPORT) + "\n";
		msg += "\t" + staticDac.getDeviceInfo() + "\n";
	}
	else {
		msg += "\tStatic AO System is disabled! Enable in \"constants.h\"\n";
	}
	msg += "Static DDS System:\n";
	if (!STATICDDS_SAFEMODE) {
		msg += str("\tStatic DDS System is Active at port") + STATICDDS_PORT + ", with baudrate " + str(STATICDDS_BAUDRATE) + "\n";
		msg += "\t" + staticDds.getDeviceInfo() + "\n";
	}
	else {
		msg += "\tStatic DDS System is disabled! Enable in \"constants.h\"\n";
	}
	msg += "Elliptec System:\n";
	if (!ELLIPTEC_SAFEMODE) {
		msg += str("\tElliptec rotation stage System is Active at port") + ELLIPTEC_PORT + ", with baudrate " + str(9600) + "\n";
		msg += "\t" + elliptec.getDeviceInfo() + "\n";
	}
	else {
		msg += "\tElliptec System is disabled! Enable in \"constants.h\"\n";
	}

	return msg;
}

void QtAnalysisWindow::windowOpenConfig(ConfigStream& configFile)
{
	try {
		ConfigSystem::standardOpenConfig(configFile, staticDac.getConfigDelim(), &staticDac);
		ConfigSystem::standardOpenConfig(configFile, staticDds.getConfigDelim(), &staticDds);
		ConfigSystem::standardOpenConfig(configFile, elliptec.getConfigDelim(), &elliptec);
	}
	catch (ChimeraError&) {
		throwNested("Analysis Window failed to read parameters from the configuration file.");
	}
}

void QtAnalysisWindow::windowSaveConfig(ConfigStream& configFile)
{
	staticDac.handleSaveConfig(configFile);
	staticDds.handleSaveConfig(configFile);
	elliptec.handleSaveConfig(configFile);
}

void QtAnalysisWindow::fillExpDeviceList(DeviceList& list)
{
	list.list.push_back(staticDac.getCore());
	list.list.push_back(staticDds.getCore());
	list.list.push_back(elliptec.getCore());
}

void QtAnalysisWindow::initializeWidgets()
{
	statBox = new ColorBox(this, mainWin->getDevices());
	QWidget* centralWidget = new QWidget(this);
	setCentralWidget(centralWidget);

	QHBoxLayout* layout = new QHBoxLayout(centralWidget);
	QVBoxLayout* layoutMOT = new QVBoxLayout(this);
	layoutMOT->setContentsMargins(0, 0, 0, 0);
	MOTAnalySys.initialize();
	for (auto& p : MOTAnalySys.MOTCalcCtrl) {
		layoutMOT->addWidget(&p, 0);
	}
	layoutMOT->addStretch(1);
	layout->addLayout(layoutMOT);

	SeqPlotter.initialize(this);
	QVBoxLayout* layoutSeq = new QVBoxLayout(this);
	layoutSeq->setContentsMargins(0, 0, 0, 0);
	for (auto* p : SeqPlotter.aoPlots) {
		layoutSeq->addWidget(p->plot);
	}
	for (auto* p : SeqPlotter.ttlPlots) {
		layoutSeq->addWidget(p->plot);
	}
	for (auto* p : SeqPlotter.olPlots) {
		layoutSeq->addWidget(p->plot);
	}
	layout->addLayout(layoutSeq);

	QVBoxLayout* layoutAux = new QVBoxLayout(this);
	layoutAux->setContentsMargins(0, 0, 0, 0);
	staticDac.initialize();
	layoutAux->addWidget(&staticDac);
	staticDds.initialize();
	layoutAux->addWidget(&staticDds);
	elliptec.initialize();
	layoutAux->addWidget(&elliptec);

	layoutAux->addStretch(1);

	layout->addLayout(layoutAux);
}

void QtAnalysisWindow::prepareCalcForAcq()
{
	MOTAnalySys.prepareMOTAnalysis();
}