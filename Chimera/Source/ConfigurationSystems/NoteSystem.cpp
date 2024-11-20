// created by Mark O. Brown
#include "stdafx.h"
#include "NoteSystem.h"
#include "LowLevel/constants.h"
#include "ConfigurationSystems/ConfigSystem.h"
#include <PrimaryWindows/QtMainWindow.h>
#include <QFile>
#include <string>
#include <qlayout.h>

void NoteSystem::handleSaveConfig(ConfigStream& saveFile){
	saveFile << "CONFIGURATION_NOTES\n";
	saveFile << getConfigurationNotes();
	saveFile << "\nEND_CONFIGURATION_NOTES\n";
}

void NoteSystem::handleOpenConfig(ConfigStream& openFile){
	std::string notes;
	auto pos = openFile.tellg ( );
	std::string tempNote = openFile.getline();
	if (tempNote != "END_CONFIGURATION_NOTES"){
		while (openFile && tempNote != "END_CONFIGURATION_NOTES"){
			notes += tempNote + "\r\n";
			pos = openFile.tellg ( );
			std::getline(openFile, tempNote);
		}
		if (size_t end = notes.find_last_not_of("\r\n");
			end != std::string::npos){
			notes = notes.substr(0, end + 1);
		}
		else {
			notes = "";
		}
		setConfigurationNotes(notes);
	}
	else{
		setConfigurationNotes("");
	}
	// for consistency with other open functions, the end delimiter will be readbtn outside this function, so go back 
	// one line
	openFile.seekg ( pos );
}

void NoteSystem::initialize(IChimeraQtWindow* win)
{
	QVBoxLayout* layout = new QVBoxLayout(this);
	layout->setContentsMargins(0, 0, 0, 0);
	header = new QLabel ("CONFIGURATION NOTES", win);
	edit = new CQTextEdit (win);
	layout->addWidget(header, 0);
	layout->addWidget(edit, 1);
	edit->installEventFilter(this);
	editZoom = 0;
}

void NoteSystem::setConfigurationNotes(std::string notes){
	edit->setText (cstr(notes));
}

std::string NoteSystem::getConfigurationNotes(){
	std::string text = str(edit->toPlainText());
	return text;
}

bool NoteSystem::eventFilter(QObject* obj, QEvent* event)
{
	auto aa = event->type();
	if (obj == edit && event->type() == QEvent::Wheel)
	{
		QWheelEvent* wheel = static_cast<QWheelEvent*>(event);
		if (wheel->modifiers() == Qt::ControlModifier)
		{
			if (wheel->delta() > 0)
			{
				edit->zoomIn();
				editZoom++;
			}

			else
			{
				if (edit->currentFont().pointSize() != 1) { editZoom--; }
				edit->zoomOut();
			}

			return true;
		}
	}
	else if (obj == edit && event->type() == QEvent::KeyPress)
	{
		QKeyEvent* key = static_cast<QKeyEvent*>(event);
		if (key->modifiers() == Qt::ControlModifier)
		{
			if (key->key() == Qt::Key_0)
			{
				edit->zoomOut(editZoom);
				//editZoom > 0 ? edit->zoomOut(editZoom) : edit->zoomIn(abs(editZoom));
				editZoom = 0;
				return true;
			}
			else if (key->key() == Qt::Key_Equal)
			{
				edit->zoomIn();
				editZoom++;
				return true;
			}
			else if (key->key() == Qt::Key_Minus)
			{
				if (edit->currentFont().pointSize() != 1) { editZoom--; }
				edit->zoomOut();
				return true;
			}

		}

	}

	return false;
}

//void NoteSystem::mouseDoubleClickEvent(QMouseEvent* event)
//{
//	if (event->modifiers() == Qt::ControlModifier)
//	{
//		editZoom > 0 ? edit->zoomOut(editZoom) : edit->zoomIn(editZoom);
//		editZoom = 0;
//	}
//	else {
//		QWidget::mouseDoubleClickEvent(event);
//	}
//}

