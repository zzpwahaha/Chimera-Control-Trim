#pragma once
#include <qobject.h>

class IChimeraSystem;
class IChimeraQtWindow;
class QtMainWindow;
class QtAndorWindow;
class QtMakoWindow;
class QtAuxiliaryWindow;
class QtScriptWindow;
class QtAnalysisWindow;
class MakoCamera;

// THIS SHOULD LIVE IN MAIN THREAD DUE TO QT GUI OPREATIONS
class CommandModulator : public QObject
{
	Q_OBJECT
public:
	CommandModulator(IChimeraSystem* parentSys);
	void initialize(IChimeraQtWindow* win);
	struct ErrorStatus {
		bool error = false;
		std::string errorMsg;
	};

public slots:
	void openConfiguration(QString addressName, ErrorStatus& status);
	void openMasterScript(QString addressName, ErrorStatus& status);
	void saveAll(ErrorStatus& status);
	void startExperiment(QString expDataName, ErrorStatus& status);
	void abortExperiment(bool keepData, QString dataName, ErrorStatus& status);
	void isExperimentRunning(bool& running, ErrorStatus& status);
	void startCalibration(QString calName, ErrorStatus& status);
	void isCalibrationRunning(bool& running, ErrorStatus& status);
	void setStaticDDS(QString ddsfreqStr, QString channelStr, ErrorStatus& status);
	void setTTL(QString name, QString value, ErrorStatus& status);
	void setDAC(QString name, QString value, ErrorStatus& status);
	void setOL(ErrorStatus& status);
	void setDDS(ErrorStatus& status);

	void startMako(QString whichMako, ErrorStatus& status);
	void stopMako(QString whichMako, ErrorStatus& status);
	void getMakoImage(QString whichMako, QVector<char>& imgResult, ErrorStatus& status);
	void getMakoImageDimension(QString whichMako, QVector<char>& imgDimParamResult, ErrorStatus& status);
	void getMakoFeatureValue(QString whichMako, QString featureName, QString featureType, QVector<char>& featureValue, ErrorStatus& status);
	void setMakoFeatureValue(QString whichMako, QString featureName, QString featureType, QString featureValue, ErrorStatus& status);
	void setPicoScrewPosition(QString whichScrew, QString value, QString bUpdate, ErrorStatus& status);
	void getPicoScrewPositions(QVector<char>& positionValue, ErrorStatus& status);
	void setPicoScrewHomes(ErrorStatus& status);

public:
	template<typename T>
	static std::vector<char> vectorToVectorChar(const std::vector<T>& data);
private:
	std::string convertToUnixPath(std::string mixedPath);
	MakoCamera* getMakoCameraPtr(QString whichMako, ErrorStatus& status);

private:
	QtMainWindow* mainWin = nullptr;
	QtScriptWindow* scriptWin = nullptr;
	QtAndorWindow* andorWin = nullptr;
	QtAuxiliaryWindow* auxWin = nullptr;
	QtMakoWindow* makoWin1 = nullptr;
	QtMakoWindow* makoWin2 = nullptr;
	QtAnalysisWindow* analysisWin = nullptr;

};
