#include "stdafx.h"
#include "StaticAoSystem.h"
#include <PrimaryWindows/IChimeraQtWindow.h>
#include <PrimaryWindows/QtMainWindow.h>
#include <PrimaryWindows/QtAuxiliaryWindow.h>
#include <qpushbutton.h>
#include <qlayout.h>

StaticAoSystem::StaticAoSystem(IChimeraQtWindow* parent) :
	IChimeraSystem(parent),
	expActive(false),
	core(STATICAO_SAFEMODE, STATICAO_IPADDRESS, STATICAO_IPPORT)
{
}

void StaticAoSystem::initialize()
{
	QVBoxLayout* layout = new QVBoxLayout(this);
	layout->setContentsMargins(0, 0, 0, 0);
	this->setMaximumWidth(600);

	QLabel* title = new QLabel("STATIC DAC", this);
	layout->addWidget(title, 0);

	QHBoxLayout* layout1 = new QHBoxLayout();
	layout1->setContentsMargins(0, 0, 0, 0);

	auto programNowButton = new QPushButton("Program DAC Now", this);
	connect(programNowButton, &QPushButton::released, [this]() {
		try {
			handleProgramNowPress(parentWin->auxWin->getUsableConstants());
		}
		catch (ChimeraError& err) {
			parentWin->reportErr("Failed to program Static AO system! \n" + err.qtrace());
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

	for (auto ch : range(size_t(StaticAOGrid::total))) {
		auto strChan = qstr(ch);
		labels[ch] = new QLabel(strChan + ":", this);
		edits[ch] = new QLineEdit(this);
		edits[ch]->setText("0.0");
		connect(edits[ch], &QLineEdit::textChanged, [this]() { parentWin->configUpdated(); });
	}

	for (auto ch : range(size_t(StaticAOGrid::total))) {
		QHBoxLayout* lay = new QHBoxLayout();
		lay->setContentsMargins(0, 0, 0, 0);
		auto strChan = qstr(ch);
		lay->addWidget(labels[ch], 0);
		lay->addWidget(edits[ch], 0);
		layout2->addLayout(lay, ch / 4, ch % 4);
	}
	layout->addLayout(layout2);

}

void StaticAoSystem::handleOpenConfig(ConfigStream& configFile)
{
	auto configVals = core.getSettingsFromConfig(configFile);
	for (auto ch : range(size_t(StaticAOGrid::total))) {
		edits[ch]->setText(qstr(configVals.staticAOs[ch].expressionStr));
	}
	ctrlButton->setChecked(configVals.ctrlAO);
	updateCtrlEnable();
}

void StaticAoSystem::handleSaveConfig(ConfigStream& configFile)
{
	configFile << core.configDelim;
	for (auto ch : range(size_t(StaticAOGrid::total))) {
		auto strChan = str(ch);
		configFile << "\n/* DAC-" + strChan + " Value:*/\t\t" << Expression(str(edits[ch]->text()));
	}
	configFile << "\n/*Control?*/\t\t\t" << ctrlButton->isChecked()
		<< "\nEND_" + core.configDelim << "\n";
}

void StaticAoSystem::updateCtrlEnable()
{
	auto ctrl = ctrlButton->isChecked();
	for (auto& e : edits) {
		e->setEnabled(!ctrl);
	}
}

void StaticAoSystem::handleProgramNowPress(std::vector<parameterType> constants)
{
	StaticAOSettings tmpSetting;
	for (auto ch : range(size_t(StaticAOGrid::total))) {
		tmpSetting.staticAOs[ch].expressionStr = str(edits[ch]->text());
	}
	tmpSetting.ctrlAO = true;

	core.setStaticAOExpSetting(tmpSetting);
	core.calculateVariations(constants, nullptr);
	core.programVariation(0, constants, nullptr);

	emit notification("Finished programming Static AO system!\n", 0);
}

std::string StaticAoSystem::getDeviceInfo()
{
	return core.getDeviceInfo();
}

