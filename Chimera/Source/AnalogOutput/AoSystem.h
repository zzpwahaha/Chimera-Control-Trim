// created by Mark O. Brown
#pragma once
#include <array>
#include <string>
#include <unordered_map>

#include "ParameterSystem/ParameterSystem.h"
//#include "AnalogOutput/DaqMxFlume.h"
#include "AnalogOutput/AoStructures.h"
#include "AnalogOutput/AnalogOutput.h"
#include "AnalogOutput/AoCore.h"
#include <GeneralObjects/IChimeraSystem.h>
#include "ConfigurationSystems/Version.h"
#include "ZynqTCP/ZynqTCP.h"

#include "qlabel.h"
#include <qpushbutton.h>
#include <qcheckbox.h>
#include <qlineedit.h>

class MainWindow;
class IChimeraQtWindow;

/**
 * AIO in the name stands for Analog In and Out, or measuring analog signals and producing analog signals.
 *
 * The AoSystem is meant to be a constant class but it currently doesn't actually prevent the user from making 
 * multiple copies of the object. This class is based off of the DAC.bas module in the original VB6 code, of course 
 * adapted for this gui in controlling the relevant controls and handling changes more directly.
 */
class AoSystem : public IChimeraSystem 
{
	Q_OBJECT
	public:
		// THIS CLASS IS NOT COPYABLE.
		AoSystem& operator=(const AoSystem&) = delete;
		AoSystem (const AoSystem&) = delete;
		AoSystem(IChimeraQtWindow* parent);

		// standard functions for gui elements
		void initialize( IChimeraQtWindow* master);
		void standardExperimentPrep (unsigned variationInc, std::vector<parameterType>& expParams,
									 double currLoadSkipTime);
		bool eventFilter (QObject* obj, QEvent* event);
		// configs
		void handleSaveConfig(ConfigStream& saveFile);
		void handleOpenConfig(ConfigStream& openFile);

		

		void handleRoundToDac( );
		void updateEdits( );
		void setDefaultValue( unsigned dacNum, double val );
		void setName( int dacNumber, std::string name );
		void setNote ( int dacNumber, std::string note );
		void setMinMax( int dacNumber, double min, double max );
		void handleEditChange( unsigned dacNumber );


		// processing to determine how dac's get set
		void handleSetDacsButtonPress( bool useDefault=false );
		void zeroDacs();

		void setDacStatus(std::array<double, size_t(AOGrid::total)> status);
		void setDacStatusNoForceOut(std::array<double, size_t(AOGrid::total)> status);
		/*below two seems need to be delete 2/16/2021 zzp*/
		void prepareDacForceChange(int line, double voltage);
	

		// getters
		double getDefaultValue( unsigned dacNum );
		std::string getName( int dacNumber );
		std::string getNote ( int dacNumber );
		double getDacValue( int dacNumber );
		std::array<double,size_t(AOGrid::total)> getDacValues();
		unsigned int getNumberOfDacs( );
		std::pair<double, double> getDacRange( int dacNumber );
		std::array<AoInfo, size_t(AOGrid::total)> getDacInfo ( );

		AoCore& getCore() { return core; }

		//zynq
		void setDACs();
		void setSingleDac(unsigned dacNumber, double val); // used for calibration, can not have gui stuff
		void setSingleDacGui(unsigned dacNumber, double val); // used for external control, can have gui stuff
		bool IsquickChange() { return quickChange->isChecked(); }
	
	signals:
		void setExperimentActiveColor(std::vector<AoCommand>, bool expFinished);


	private:


		QLabel* dacTitle;
		CQPushButton* dacSetButton;
		CQPushButton* zeroDacsButton;
		CQCheckBox* quickChange;

		std::array<AnalogOutput, size_t(AOGrid::total)> outputs;
		AoCore core;

		double dacTriggerTime;
		bool roundToDacPrecision;


		//Zynq tcp connection
		ZynqTCP zynq_tcp;

		static constexpr double dacResolution = 20.0 / 0xffff; /*16bit dac*/
		const int numDigits = static_cast<int>(abs(round(log10(dacResolution) - 0.49)));

};
Q_DECLARE_METATYPE(std::vector<AoCommand>);

