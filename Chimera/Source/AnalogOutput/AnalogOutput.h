// created by Mark O. Brown
#pragma once
//#include "control.h"
#include "AoStructures.h"
#include <CustomQtControls/AutoNotifyCtrls.h>
#include <qlabel.h>
#include <qlayout.h>

class IChimeraQtWindow;

class AnalogOutput
{

public:
	AnalogOutput ( );
	virtual void initialize ( IChimeraQtWindow* parent, int whichDac );
	void handleEdit(bool roundToDacPrecision = false);
	void updateEdit(bool roundToDacPrecision = false);
	double roundToDacResolution ( double num );
	double getVal ( bool useDefault );
	bool eventFilter (QObject* obj, QEvent* event);
	void setNote ( std::string note );
	AoInfo info;
	void setName ( std::string name );
	void disable ( );
	void setExpActiveColor(bool usedInExp, bool expFinished = false);
	double checkBound(double dacVal);
	bool isEdit(QObject* obj);
	QHBoxLayout* getLayout() const { return layout; }

	virtual double dacResolution() { return dacResolutionInst; }
	virtual int numDigits() { return numDigitsInst; }

protected:
	unsigned dacNum;
	CQLineEdit* edit;
	QLabel* label;
	QHBoxLayout* layout;

public:
	static constexpr double dacResolutionInst = 20.0 / 0xffff; /*16bit dac*/
	const int numDigitsInst = static_cast<int>(abs(round(log10(dacResolutionInst) - 0.49)));
};

//AnalogOutput::numDigits = static_cast<int>(abs(round(log10(dacResolution) - 0.49)));