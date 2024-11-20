// created by Mark O. Brown
#pragma once
#include "GeneralUtilityFunctions/miscCommonFunctions.h"
#include "DigitalOutput/DoStructures.h"
#include "ParameterSystem/Expression.h"
#include "Plotting/PlotInfo.h"
#include "GeneralFlumes/ftdiStructures.h"
#include "ConfigurationSystems/Version.h"
#include "ConfigurationSystems/ConfigStream.h"
#include "GeneralObjects/ExpWrap.h"
#include "GeneralObjects/Matrix.h"

#include "DigitalOutput.h"
#include "DoCore.h"

#include <array>
#include <sstream>
#include <unordered_map>
#include <qlabel.h>
#include <qpushbutton.h>
#include <GeneralObjects/IChimeraSystem.h>

/**/
class IChimeraQtWindow;
class AuxiliaryWindow;

class DoSystem : public IChimeraSystem
{
	Q_OBJECT
	public:
		// THIS CLASS IS NOT COPYABLE.
		DoSystem& operator=(const DoSystem&) = delete;
		DoSystem (const DoSystem&) = delete;

		DoSystem (IChimeraQtWindow* parent);
		~DoSystem();
		/// config handling
		void handleSaveConfig(ConfigStream& saveFile);
		void handleOpenConfig(ConfigStream& openFile);
		void initialize( IChimeraQtWindow* master );
		int getNumberOfTTLRows();
		int getNumberOfTTLsPerRow();
		void zeroBoard();
		void handleTTLPress (DigitalOutput& out);
		void handleHoldPress();
		void setTtlStatus(DOStatus status);
		void setTtlStatusNoForceOut(DOStatus status);
		//bool getFtFlumeSafemode ();

		std::pair<unsigned, unsigned> getTtlBoardSize();

		void setName(unsigned row, unsigned number, std::string name);

		std::string getName(unsigned row, unsigned number);


		std::array<std::string, size_t(DOGrid::total)> getAllNames ();
		bool getTtlStatus (unsigned row, int number );
		void updateDefaultTtl(unsigned row, unsigned column, bool state);
		bool getDefaultTtl(unsigned row, unsigned column);
		void setSingleTtlGui(unsigned row, unsigned number, bool val); // used for external control, can have gui stuff

		DOStatus getCurrentStatus( );

		allDigitalOutputs& getDigitalOutputs();
		DoCore& getCore ();

		void standardExperimentPrep(unsigned variationInc, double currLoadSkipTime, std::vector<parameterType>& expParams);

	signals:
		void setExperimentActiveColor(std::vector<DoCommand>, bool expFinished);

	private:
		DoCore core;
		/// other.
		// one control for each TTL
		QLabel* ttlTitle;
		CQPushButton* ttlHold;
		CQPushButton* zeroTtls;
		allDigitalOutputs outputs;

		// tells whether the hold button is down or not.
		bool holdStatus; 
};
Q_DECLARE_METATYPE(std::vector<DoCommand>);
