// created by Mark O. Brown
#include "stdafx.h"
#include "StatusControl.h"
#include <PrimaryWindows/IChimeraQtWindow.h>
#include <qlayout.h>
#include "StatusOptionsWindow.h"

void StatusControl::initialize(IChimeraQtWindow* parent, std::string headerText, std::vector<std::string> textColors)
{
	
	QVBoxLayout* layout = new QVBoxLayout(this);
	QHBoxLayout* layout1 = new QHBoxLayout();
	layout1->setContentsMargins(0, 0, 0, 0);
	layout->setContentsMargins(0, 0, 0, 0);
	this->setMaximumWidth(widgetWidthMax);

	if (textColors.size () == 0) {
		thrower ("Need to set a nonzero number of colors for status control!");
	}
	colors = textColors;
	//defaultColor = textColor;
	header = new QLabel (headerText.c_str (), parent);	
	options = new QPushButton("Options", parent);
	connect(options, &QPushButton::pressed, [this, parent]() {
		try {
			// edit existing plot file using the plot designer.
			StatusOptionsWindow dialog = StatusOptionsWindow(parent, &opts, msgHistory);
			dialog.setStyleSheet(chimeraStyleSheets::stdStyleSheet());
			dialog.exec();
			// resize history
			while (msgHistory.size() > opts.historyLength) {
				msgHistory.pop_front();
			}
		}
		catch (ChimeraError& err) {
			errBox(err.trace());
		}});

	redrawBtn = new QPushButton("Fancy Redraw", parent);
	redrawBtn->connect(redrawBtn, &QPushButton::pressed, [this]() {
		redrawControl();
		});

	clearBtn = new QPushButton (parent);
	clearBtn->setText ("Clear");
	edit = new QTextEdit (parent);
	edit->setReadOnly(true);
	edit->setStyleSheet("QPlainTextEdit { color: " + qstr(textColors[0]) + "; }");
	parent->connect(clearBtn, &QPushButton::released, [this]() {clear(); });

	layout1->addWidget(header, 0);
	layout1->addWidget(options, 0);
	layout1->addWidget(redrawBtn, 0);
	layout1->addWidget(clearBtn, 0);
	layout->addLayout(layout1, 0);
	layout->addWidget(edit, 1);
	
}

void StatusControl::addStatusToQue(statusMsg newMsg) {
	msgHistory.push_back(newMsg);
	if (msgHistory.size() > opts.historyLength) {
		msgHistory.pop_front();
	}
}

void StatusControl::redrawControl() {
	edit->clear();
	for (auto msg : msgHistory) {
		// importantly add here without re-adding to queue. 
		addHtmlStatusTextInner(msg);
	}
}

void StatusControl::addStatusText(statusMsg msg) {
	addStatusToQue(msg);
	addPlainStatusTextInner(msg);
}

void StatusControl::addPlainStatusTextInner(statusMsg msg) {
	indvOption indvOpt;
	for (auto keyv : opts.indvOptions.keys()) {
		if (msg.systemDelim == keyv) {
			indvOpt = opts.indvOptions.value(keyv);
		}
	}
	if (opts.debugLvl >= msg.baseLevel + indvOpt.debugLvlOffset && indvOpt.show) {
		std::string finText = str(msg.msg);
		if (opts.showOrigin) {
			finText = "[" + str(msg.systemDelim) + "]: " + finText;
		}
		for (auto lvl : range(msg.baseLevel)) {
			// visual indication of what level a message is.
			finText = ">" + finText;
		}
		addPlainText(finText);
	}
}

void StatusControl::addHtmlStatusTextInner(statusMsg msg) {
	if (colors.size() == 0) {
		return;
	}
	indvOption indvOpt;
	for (auto keyv : opts.indvOptions.keys()) {
		if (msg.systemDelim == keyv) {
			indvOpt = opts.indvOptions.value(keyv);
		}
	}
	if (opts.debugLvl >= msg.baseLevel + indvOpt.debugLvlOffset && indvOpt.show) {
		std::string finText = str(msg.msg);
		if (opts.showOrigin) {
			finText = "[" + str(msg.systemDelim) + "]: " + finText;
		}
		for (auto lvl : range(msg.baseLevel)) {
			// visual indication of what level a message is.
			finText = ">" + finText;
		}
		if (finText.substr(0, 11) == "**********") {
			addHtmlStatusText(finText, "#FFFFFF");
		}
		addHtmlStatusText(finText, msg.baseLevel > colors.size() ? colors.back() : colors[msg.baseLevel]);
	}
}

void StatusControl::addHtmlStatusText(std::string text, std::string color) {
	QString htmlTxt = ("<font color = \"" + color + "\">" + text + "</font>").c_str();
	//e.g. <font color = "red">This is some text!< / font>
	htmlTxt.replace("\r", "");
	htmlTxt.replace("\n", "<br/>");
	// html is weird and doesn't have proper tab support. 
	htmlTxt.replace("\t", "&nbsp;&nbsp;&nbsp;&nbsp;");
	// de-emphasize the location information.
	htmlTxt.replace("{", "</font><font color = \"#000000\">{");
	htmlTxt.replace("}", qstr("}</font><font color = \"" + color + "\">"));
	/*if (htmlTxt.lastIndexOf("@ Location:") != -1) {
		htmlTxt.replace("@ Location:", "</font><font color = \"#000000\">@ Location:");
	}*/
	edit->moveCursor(QTextCursor::End);
	edit->textCursor().insertHtml(htmlTxt);
	edit->moveCursor(QTextCursor::End);
}

void StatusControl::addPlainText(std::string text) {
	if (!edit) {
		return;
	}
	edit->insertPlainText(qstr(text));
	edit->moveCursor(QTextCursor::End);
}

void StatusControl::clear() {
	edit->clear();
	addHtmlStatusText("**************CLEARED****************\r\n", "#FFFFFF");
}


void StatusControl::appendTimebar() {
	time_t time_obj = time(0);   // get time now
	struct tm currentTime;
	localtime_s(&currentTime, &time_obj);
	std::string timeStr = "(" + str(currentTime.tm_year + 1900) + ":" + str(currentTime.tm_mon + 1) + ":"
		+ str(currentTime.tm_mday) + ") " + str(currentTime.tm_hour) + ":"
		+ str(currentTime.tm_min) + ":" + str(currentTime.tm_sec);
	statusMsg timebarMsg;
	timebarMsg.msg = qstr("\r\n**********" + timeStr + "**********\r\n");
	addStatusText(timebarMsg);
}

