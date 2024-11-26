#include "stdafx.h"
#include "ElliptecSystem.h"
#include <PrimaryWindows/IChimeraQtWindow.h>
#include <PrimaryWindows/QtMainWindow.h>
#include <PrimaryWindows/QtAuxiliaryWindow.h>
#include <boost/format.hpp>
#include <qpushbutton.h>
#include <qlayout.h>

ElliptecSystem::ElliptecSystem(IChimeraQtWindow* parent) : 
	IChimeraSystem(parent),
	expActive(false),
	core(ELLIPTEC_SAFEMODE, ELLIPTEC_PORT)
{}

void ElliptecSystem::initialize()
{
	QVBoxLayout* layout = new QVBoxLayout(this);
	layout->setContentsMargins(0, 0, 0, 0);
	this->setMaximumWidth(600);

	QLabel* title = new QLabel("ELLIPTEC", this);
	layout->addWidget(title, 0);

	QHBoxLayout* layout1 = new QHBoxLayout();
	layout1->setContentsMargins(0, 0, 0, 0);

	auto programNowButton = new QPushButton("Program ELLIPTEC Now", this);
	connect(programNowButton, &QPushButton::released, [this]() {
		try {
			handleProgramNowPress(parentWin->auxWin->getUsableConstants());
		}
		catch (ChimeraError& err) {
			parentWin->reportErr("Failed to program ELLIPTEC system! \n" + err.qtrace());
		}
		});
	ctrlButton = new QCheckBox("Ctrl?", this);
	ctrlButton->setChecked(false);
	connect(ctrlButton, &QCheckBox::clicked, [this]() {
		try {
			updateCtrlEnable();
			parentWin->configUpdated();
		}
		catch (ChimeraError& err) {
			parentWin->reportErr(err.qtrace());
		}
		});

	layout1->addWidget(programNowButton, 0);
	layout1->addWidget(ctrlButton, 0);
	layout1->addStretch(1);
	layout->addLayout(layout1, 0);

	QGridLayout* layout2 = new QGridLayout();
	layout2->setContentsMargins(0, 0, 0, 0);

	for (auto ch : range(size_t(ElliptecGrid::total))) {
		auto strChan = qstr(ch);
		labels[ch] = new QLabel(strChan + ":" + (boost::format("%8.3f") % 0.0).str().c_str(), this);
		edits[ch] = new QLineEdit(this);
		edits[ch]->setText("0.0");
		connect(edits[ch], &QLineEdit::textChanged, [this]() { parentWin->configUpdated(); });
	}
	for (auto ch : range(size_t(ElliptecGrid::total))) {
		QHBoxLayout* lay = new QHBoxLayout();
		lay->setContentsMargins(0, 0, 0, 0);
		auto strChan = qstr(ch);
		lay->addWidget(labels[ch], 0);
		lay->addWidget(edits[ch], 0);
		lay->addStretch(1);
		layout2->addLayout(lay, ch / 4, ch % 4);
	}
	layout->addLayout(layout2);

	connect(&core, &ElliptecCore::currentAngle, this, [this](unsigned id, double angle) {
		labels[id]->setText((boost::format("%u:%8.3f") % id % angle).str().c_str());
		});

	QTimer* timer = new QTimer(this);
	connect(timer, &QTimer::timeout, [this]() {
		try {
			if (!parentWin->mainWin->expIsRunning()) {
				updateCurrentValues();
			}
		}
		catch (ChimeraError&) {}
		});
	// could probably make this time a front panel option.
	timer->start(5000);

}

void ElliptecSystem::handleOpenConfig(ConfigStream& configFile)
{
	auto configVals = core.getSettingsFromConfig(configFile);
	for (auto ch : range(size_t(ElliptecGrid::total))) {
		edits[ch]->setText(qstr(configVals.elliptecs[ch].expressionStr));
	}
	ctrlButton->setChecked(configVals.ctrlEll);
	updateCtrlEnable();
}

void ElliptecSystem::handleSaveConfig(ConfigStream& configFile)
{
	configFile << core.configDelim;
	for (auto ch : range(size_t(ElliptecGrid::total))) {
		auto strChan = str(ch);
		configFile << "\n/* Elliptec-" + strChan + " Value:*/\t\t" << Expression(str(edits[ch]->text()));
	}
	configFile << "\n/*Control?*/\t\t\t" << ctrlButton->isChecked()
		<< "\nEND_" + core.configDelim << "\n";
}

void ElliptecSystem::updateCtrlEnable()
{
	auto ctrl = ctrlButton->isChecked();
	for (auto& e : edits) {
		e->setEnabled(!ctrl);
	}
}

void ElliptecSystem::handleProgramNowPress(std::vector<parameterType> constants)
{
	ElliptecSettings tmpSetting;
	for (auto ch : range(size_t(ElliptecGrid::total))) {
		tmpSetting.elliptecs[ch].expressionStr = str(edits[ch]->text());
	}
	tmpSetting.ctrlEll = true;

	core.setElliptecExpSettings(tmpSetting);
	core.calculateVariations(constants, nullptr);
	core.programVariation(0, constants, nullptr);

	emit notification("Finished programming Elliptec system!\n", 0);
}

std::string ElliptecSystem::getDeviceInfo()
{
	return core.getDeviceInfo();
}

void ElliptecSystem::updateCurrentValues()
{
	for (auto ch : range(size_t(ElliptecGrid::total))) {
		double angle = core.getElliptecPosition(ch);
		labels[ch]->setText((boost::format("%u:%8.3f") % ch % angle).str().c_str());
	}
}


