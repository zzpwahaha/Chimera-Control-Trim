// created by Mark O. Brown

#include "stdafx.h"
#include "MainOptionsControl.h"
#include <PrimaryWindows/IChimeraQtWindow.h>
#include "ConfigurationSystems/ConfigSystem.h"
#include "PrimaryWindows/QtMainWindow.h"
#include <boost/lexical_cast.hpp>
#include <qdebug.h>
#include <qlayout.h>

void MainOptionsControl::initialize( IChimeraQtWindow* parent )
{
	QVBoxLayout* layout = new QVBoxLayout(this);
	layout->setContentsMargins(0, 0, 0, 0);

	header = new QLabel ("MAIN OPTIONS", parent);
	auto configUpdate = [parent]() {parent->configUpdated (); };
	auto changeBkgColor = [](QCheckBox* chkBox) { chkBox->isChecked() ?
		chkBox->setStyleSheet("QCheckBox { background-color: chartreuse }")
		: chkBox->setStyleSheet("QCheckBox { background-color: (240,240,240) }"); };

	randomizeVariationsButton = new QCheckBox ("Randomize Variations?", parent);
	parent->connect (randomizeVariationsButton, &QCheckBox::stateChanged, configUpdate);
	connect(randomizeVariationsButton, &QCheckBox::stateChanged, [this, changeBkgColor]() {
		changeBkgColor(randomizeVariationsButton); });

	repetitionFirstButton = new QCheckBox("Repetition First?", parent);
	parent->connect(repetitionFirstButton, &QCheckBox::stateChanged, configUpdate);
	connect(repetitionFirstButton, &QCheckBox::stateChanged, [this, changeBkgColor]() {
		changeBkgColor(repetitionFirstButton); });

	inExpCalControl = new InExpCalControl(parent);
	inExpCalControl->initialize(parent);

	delayAutoCal = new QCheckBox ("Delay Auto-Calibration", parent);
	connect(delayAutoCal, &QCheckBox::stateChanged, [this, changeBkgColor]() {
		changeBkgColor(delayAutoCal); });
	delayAutoCal->setChecked(true);

	layout->addWidget(header, 0);
	layout->addWidget(randomizeVariationsButton, 0);
	layout->addWidget(repetitionFirstButton, 0);
	layout->addLayout(inExpCalControl->layout);
	layout->addWidget(delayAutoCal, 0);

	QHBoxLayout* layout1 = new QHBoxLayout();
	layout1->setContentsMargins(0, 0, 0, 0);
	atomThresholdForSkipText = new QLabel ("Atom Threshold for Load Skip:", parent);
	atomThresholdForSkipEdit = new QLineEdit ("-1", parent);
	parent->connect (atomThresholdForSkipEdit, &QLineEdit::textChanged, configUpdate);

	layout1->addWidget(atomThresholdForSkipText, 0);
	layout1->addWidget(atomThresholdForSkipEdit, 1);

	layout->addLayout(layout1);

}

void MainOptionsControl::handleSaveConfig(ConfigStream& saveFile){
	saveFile << "MAIN_OPTIONS"
			 << "\n/*Randomize Variations?*/ " << randomizeVariationsButton->isChecked()
			 << "\n/*Repetition First Over Variation?*/ " << repetitionFirstButton->isChecked()
			 << "\n/*In-Experiment Calibration?*/ " << inExpCalControl->inExpCalibrationButton->isChecked()
			 << "\n/*In-Experiment Calibration Interval*/ " << str(inExpCalControl->inExpCalibrationDurationEdit->text())
			 << "\n/*Atom Threshold for Load Skip*/ " << str(atomThresholdForSkipEdit->text());
	saveFile << "\nEND_MAIN_OPTIONS\n";
}

mainOptions MainOptionsControl::getSettingsFromConfig (ConfigStream& openFile ){
	mainOptions options;
	openFile >> options.randomizeVariations;
	openFile >> options.repetitionFirst;
	openFile >> options.inExpCalibration;
	std::string txt;
	openFile >> txt;
	try {
		options.inExpCalInterval = boost::lexical_cast<double>(txt);
	}
	catch (boost::bad_lexical_cast&) {
		errBox("Bad value for calibration interval value! Falls back to the default value of 10 minute");
		options.inExpCalInterval = 10.0;
	}

	openFile >> txt;
	try{
		options.atomSkipThreshold = boost::lexical_cast<unsigned long>( txt );
	}
	catch ( boost::bad_lexical_cast& ){
		errBox ( "atom threshold for load skip failed to convert to an unsigned long! The code will force "
					"the threshold to the maximum threshold." );
		options.atomSkipThreshold = -1;
	}
	return options;
}

void MainOptionsControl::setOptions ( mainOptions opts ){
	currentOptions = opts;
	randomizeVariationsButton->setChecked( currentOptions.randomizeVariations );
	repetitionFirstButton->setChecked(currentOptions.repetitionFirst);
	inExpCalControl->inExpCalibrationButton->setChecked(currentOptions.inExpCalibration);
	inExpCalControl->inExpCalibrationDurationEdit->setText(qstr(currentOptions.inExpCalInterval, 2));
	atomThresholdForSkipEdit->setText( cstr ( currentOptions.atomSkipThreshold ) );
}

mainOptions MainOptionsControl::getOptions(){
	currentOptions.randomizeVariations = randomizeVariationsButton->isChecked();
	currentOptions.repetitionFirst= repetitionFirstButton->isChecked();
	currentOptions.inExpCalibration = inExpCalControl->inExpCalibrationButton->isChecked();
	currentOptions.inExpCalInterval = inExpCalControl->interval;
	currentOptions.delayAutoCal = delayAutoCal->isChecked ();
	try{
		currentOptions.atomSkipThreshold = boost::lexical_cast<unsigned long>( str( atomThresholdForSkipEdit->text() ) );
	}
	catch ( boost::bad_lexical_cast& ){
		throwNested ( "failed to convert atom threshold for load-skip to an unsigned long!" );
	}
	return currentOptions;
}
