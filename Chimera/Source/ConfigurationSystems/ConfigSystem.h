// created by Mark O. Brown
#pragma once
#include "Control.h"
#include "GeneralObjects/commonTypes.h"
#include "profileSettings.h"
#include "ConfigurationSystems/ConfigStream.h"
#include "Scripts/ScriptStream.h"
#include <vector>
#include <string>
#include "Version.h"
#include "GeneralObjects/IDeviceCore.h"
#include "PrimaryWindows/IChimeraQtWindow.h"
#include <QPushButton>
#include <QLabel>
#include <QCheckBox>
#include <QComboBox.h>
#include <GeneralObjects/IChimeraSystem.h>

/*
    This singleton class manages the entire "profile" system, where "profiles" are my term for the entirety of the 
	settings in the code (strange word choice I	know). It consists of the relevant controls, some saved indicators 
	that can be checked to determine if the user should be prompted to save at a given point, and all of the functions 
	for saving, renaming, deleting, and creating new levels within the code. 
*/
class ConfigSystem : public IChimeraSystem
{
	Q_OBJECT
	public:
		static constexpr auto CONFIG_EXTENSION = "Config";

		ConfigSystem(std::string fileSystemPath, IChimeraQtWindow* parent);

		void checkSaveEntireProfile(IChimeraQtWindow* win);
		static std::function<void (ScriptStream&, std::string&)> getGetlineFunc (Version& ver);

		void saveConfiguration(IChimeraQtWindow* win, bool askOverwrite = true);
		void saveConfigurationAs(IChimeraQtWindow* win);
		void renameConfiguration();
		void deleteConfiguration();
		void openConfigFromPath( std::string pathToConfig, IChimeraQtWindow* win);
		static void getVersionFromFile( ConfigStream& file );
		static std::string getMasterAddressFromConfig( profileSettings profile );
		void updateConfigurationSavedStatus( bool isSaved );
		bool configurationSettingsReadyCheck(IChimeraQtWindow* win);
		bool checkConfigurationSave(std::string prompt, IChimeraQtWindow* win);
		profileSettings getProfileSettings();

		static std::vector<std::string> searchForFiles(std::string locationToSearch, std::string extensions);
		static void reloadCombo (QComboBox* combo, std::string locationToLook, std::string extension,
								  std::string nameToLoad );
		bool fileOrFolderExists ( std::string filePathway );
		void initialize(IChimeraQtWindow* win);
		void handleSelectConfigButton(IChimeraQtWindow* win);
		
		template <class sysType>
		static void standardOpenConfig ( ConfigStream& openFile, std::string delim, std::string endDelim, 
										 sysType* this_in, Version minVer = Version("0.0"));
		// an overload with the default end delimiter: "END_" + delim. Basically everything /should/ use the default.
		template <class sysType>
		static void standardOpenConfig (ConfigStream& openFile, std::string delim, sysType* this_in,
										 Version minVer = Version ( "0.0" ) );
		static void checkDelimiterLine (ConfigStream& openFile, std::string keyword );
		static bool checkDelimiterLine(ConfigStream& openFile, std::string delimiter, std::string breakCondition );
		static void jumpToDelimiter (ConfigStream& openFile, std::string delimiter );
		static void initializeAtDelim ( ConfigStream& openFile, std::string delimiter, Version minVer=Version("0.0") );
		template <class coreType, class returnT>
		static void stdGetFromConfig ( ConfigStream& configStream, coreType& sys, returnT& settings, 
									   Version minVer=Version("0.0"));
		template <class returnType>
		static returnType stdConfigGetter (ConfigStream& configStream, std::string delim,
			returnType (*getterFunc)(ConfigStream&), Version minVer=Version("0.0"));
	private:
		profileSettings currentProfile; 
		std::string FILE_SYSTEM_PATH;
		bool configurationIsSaved;
		// version = str(versionMain) + "." + str(versionSub)
		// I try to use version sub changes for small changes and version main for big formatting (sometimes backwards 
		// incompatible) changes

		// version 1.0: Start of minimalist chimera versioning. add notes here as you change the version to document
		// what the new changes. 
		const Version version = Version( "1.0" );

		QCheckBox* configurationSavedIndicator;
		QPushButton* selectConfigButton;
		QLabel* configDisplay;
};

template <class sysType>
static void ConfigSystem::standardOpenConfig (ConfigStream& openFile, std::string delim, sysType* this_in, Version minVer ){
	ConfigSystem::standardOpenConfig ( openFile, delim, "END_" + delim, this_in, minVer );
}

template <class sysType>
static void ConfigSystem::standardOpenConfig ( ConfigStream& configStream, std::string delim, std::string endDelim, 
												sysType* this_in, Version minVer ){
	// sysType must have a member function of the form
	// void ( *sysType::openFunction )( ConfigStream& f )
	try	{
		ConfigSystem::initializeAtDelim ( configStream, delim, minVer );
	}
	catch ( ChimeraError& e ){
		errBox ( "Failed to initialize config file for " + delim + "!\n\n" + e.trace ( ) );
		return;
	}
	try{
		this_in->handleOpenConfig( configStream );
	}
	catch ( ChimeraError& e ){
		errBox ( "Failed to gather information from config file for " + delim + "!\n\n" + e.trace ( ) );
		return;
	}
	try{
		ConfigSystem::checkDelimiterLine ( configStream, endDelim );
	}
	catch ( ChimeraError& e ){
		errBox ( "End delimiter for the " + delim + " control was not found. This might indicate that the "
				 "control did not initialize properly.\n\n" + e.trace() );
	}
}


template <class returnType>
static returnType ConfigSystem::stdConfigGetter (ConfigStream& configStream, std::string delim,
												  returnType (*getterFunc)(ConfigStream&), Version minVer){
	// a template functor. The getter here should get whatever is wanted from the file and return it. 
	// return type must have a default constructor so that the function knows what to do if fails.
	returnType res = returnType ();
	try{
		ConfigSystem::initializeAtDelim (configStream, delim, minVer);
	}
	catch (ChimeraError & e){
		throwNested ("Failed to initialize config file for " + delim + "!\n\n");
		return res;
	}
	try{
		res = (*getterFunc) (configStream);
	}
	catch (ChimeraError & e){
		throwNested ("Failed to gather information from config file for " + delim + "!\n\n");
		return res;
	}
	try{
		ConfigSystem::checkDelimiterLine (configStream, "END_" + delim);
	}
	catch (ChimeraError & e){
		throwNested ( "End delimiter for the " + delim + " control was not found. This might indicate that the "
					  "control did not initialize properly.\n\n");
	}
	return res;
}

template <class coreType, class returnT>
static void ConfigSystem::stdGetFromConfig ( ConfigStream& configStream, coreType& core, returnT& settings, 
	Version minVer){
	// a template functor. The getter here should get whatever is wanted from the file and return it. 
	try{
		ConfigSystem::initializeAtDelim (configStream, core.getDelim (), minVer);
	}
	catch (ChimeraError & e){
		throwNested ("Failed to initialize config file for " + core.getDelim () + "!\n\n");
	}
	try{
		settings = core.getSettingsFromConfig (configStream);
	}
	catch (ChimeraError & e){
		throwNested ("Failed to gather information from config file for " + core.getDelim() + "!\n\n");
	}
	try{
		ConfigSystem::checkDelimiterLine (configStream, "END_" + core.getDelim ());
	}
	catch (ChimeraError & e){
		throwNested ("End delimiter for the " + core.getDelim () + " control was not found. This might indicate that the "
					 "control did not initialize properly.\n\n");
	}
}

