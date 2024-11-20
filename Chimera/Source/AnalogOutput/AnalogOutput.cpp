// created by Mark O. Brown
#include "stdafx.h"
#include "AnalogOutput.h"
#include "PrimaryWindows/IChimeraQtWindow.h"
#include "boost\lexical_cast.hpp"
#include <QlineEdit>
#include <QKeyEvent>


AnalogOutput::AnalogOutput( ) {}

void AnalogOutput::initialize ( IChimeraQtWindow* parent, int whichDac) 
{
	info.name = "dac" +
		str(whichDac / size_t(AOGrid::numPERunit)) + "_" +
		str(whichDac % size_t(AOGrid::numPERunit)); /*default name, always accepted by script*/

	layout = new QHBoxLayout();
	layout->setContentsMargins(0, 0, 0, 0);
	
	label = new QLabel(QString("%1").arg(QString::number(whichDac % size_t(AOGrid::numPERunit)),2), parent);
	label->setToolTip(qstr(info.name + ": [" + str(info.minVal, numDigits(), true) + "," + str(info.maxVal, numDigits(), true) + "]" + "\r\n" + info.note));

	edit = new CQLineEdit ("0", parent);
	edit->setToolTip(qstr(info.name + ": [" + str(info.minVal, numDigits(), true) + "," + str(info.maxVal, numDigits(), true) + "]" + "\r\n" + info.note));
	edit->installEventFilter (parent);
	parent->connect (edit, &QLineEdit::textEdited,
		[this, parent]() {
			handleEdit ();
		});
	//edit->setStyleSheet ("QLineEdit { border: none }");

	layout->addWidget(label, 0, Qt::AlignRight);
	layout->addWidget(edit, 1, Qt::AlignLeft);
	//layout->addStretch(1);
	edit->setMaximumWidth(150);
	updateEdit(false);
}

bool AnalogOutput::eventFilter (QObject* obj, QEvent* event) {
	if (obj == edit) {
		if (event->type () == QEvent::KeyPress)
		{
			QKeyEvent* keyEvent = static_cast<QKeyEvent*>(event);
			bool up = true;
			if (keyEvent->key () == Qt::Key_Up){
				up = true;
			}
			else if (keyEvent->key () == Qt::Key_Down){
				up = false;
			}
			else {
				handleEdit();
				return false;
			}
			auto txt = edit->text ();
			// make sure value in edit matches current value, else unclear what this functionality should do.
			{
				double val;
				try {
					val = boost::lexical_cast<double>(str(txt));
				}
				catch (boost::bad_lexical_cast) {
					return true;
				}
				if (fabs(val - info.currVal) > 1e-12) {
					handleEdit();
					return true;
				}
			}

			auto decimalPos = txt.indexOf (".");
			auto cursorPos = edit->cursorPosition ();
			if (decimalPos == -1){
				// value is an integer, decimal is effectively at end.
				decimalPos = txt.size ();
			}
			// the order of the first digit
			double size = pow (10, decimalPos - 1);
			// handle the extra decimal character with the ternary operator here. 
			int editPlace = (cursorPos > decimalPos ? cursorPos - 1 : cursorPos);
			double inc = size / pow(10, editPlace - 1);
			info.currVal += up ? inc : -inc;

			updateEdit ();

			auto newTxt = edit->text (); 
			if (txt.indexOf ("-") == -1 && newTxt.indexOf("-") != -1){
				// then need to shift cursor to account for the negative.
				edit->setCursorPosition (cursorPos+1);
			}
			else if (txt.indexOf("-") != std::string::npos && newTxt.indexOf ("-") == std::string::npos){
				edit->setCursorPosition (cursorPos-1);
			}
			else {
				edit->setCursorPosition (cursorPos);
			}

			updateEdit();

			return true;
		}
		return false;
	}
	return false;
}

double AnalogOutput::getVal ( bool useDefault )
{
	double val;
	try	{
		val = boost::lexical_cast<double>( str (edit->text () ) );
	}
	catch ( boost::bad_lexical_cast& ){
		if ( useDefault ){
			val = 0;
		}
		else{
			throwNested ( "value entered in DAC #" + str ( dacNum ) + " (" + str(edit->text())
						  + ") failed to convert to a double!" );
		}
	}
	val = checkBound(val);
	return val;
}


void AnalogOutput::updateEdit ( bool roundToDacPrecision )
{
	std::string valStr = roundToDacPrecision ? str ( roundToDacResolution ( info.currVal ), 13, true, false, true )
		: str ( info.currVal, numDigits(), false, false, false );
	int pos = edit->cursorPosition ();
	edit->setText(valStr.c_str());
	edit->setCursorPosition (pos);
}

void AnalogOutput::setName ( std::string name ){
	if ( name == "" ){
		// no empty names allowed.
		return;
	}
	std::transform ( name.begin ( ), name.end ( ), name.begin ( ), ::tolower );
	info.name = name;
	edit->setToolTip(qstr(info.name + ": [" + str(info.minVal, numDigits(), true) + "," + str(info.maxVal, numDigits(), true) + "]" + "\r\n" + info.note));
}


void AnalogOutput::handleEdit ( bool roundToDacPrecision )
{
	double val = getVal(true);
	info.currVal = val;
}

double AnalogOutput::roundToDacResolution ( double num )
{
	//double dacResolution = 10.0 / pow ( 2, 16 );
	return long ( ( num + dacResolution() / 2 ) / dacResolution()) * dacResolution();
}


void AnalogOutput::setNote ( std::string note ){
	info.note = note;
	edit->setToolTip(qstr(info.name + ": [" + str(info.minVal, numDigits(), true) + "," + str(info.maxVal, numDigits(), true) + "]" + "\r\n" + info.note));
}

void AnalogOutput::disable ( ){
	edit->setEnabled (false);
}

void AnalogOutput::setExpActiveColor(bool usedInExp, bool expFinished)
{
	for (auto check : { edit }) {
		check->setStyleSheet(usedInExp ?
			(expFinished ? "QLineEdit { background: rgb(255, 255, 0)}" : "QLineEdit { background: rgb(0, 255, 0)}") :
			"QLineEdit { background: rgb(255, 255, 255)}");
	}
}

double AnalogOutput::checkBound(double dacVal)
{
	if (dacVal > info.maxVal)
	{
		return info.maxVal;
	}
	else if (dacVal < info.minVal)
	{
		return info.minVal;
	}
	else 
	{
		return dacVal;
	}
}

bool AnalogOutput::isEdit(QObject* obj)
{
	return obj == edit;
}
