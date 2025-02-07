#pragma once
#include "GeneralObjects/IDeviceCore.h"
#include "FrameObserver.h"
#include "MakoWrapper.h"
#include "MakoSettingControl.h"
#include "CMOSSetting.h"
#include <utility>

class IChimeraSystem;

class MakoCameraCore : public IDeviceCore
{
	Q_OBJECT
public:
	// THIS CLASS IS NOT COPYABLE.
	MakoCameraCore& operator=(const MakoCameraCore&) = delete;
	MakoCameraCore(const MakoCameraCore&) = delete;

	MakoCameraCore(CameraInfo camInfo, IChimeraSystem* parent);
	~MakoCameraCore();
	
	std::string getDelim() override { return camInfo.delim; }
	void logSettings(DataLogger& logger, ExpThreadWorker* threadworker) override;
	void loadExpSettings(ConfigStream& stream) override;
	void calculateVariations(std::vector<parameterType>& params, ExpThreadWorker* threadworker) override;
	void programVariation(unsigned variation, std::vector<parameterType>& params,
		ExpThreadWorker* threadworker) override {};
	void normalFinish() override;
	void errorFinish() override;
	MakoSettings getSettingsFromConfig(ConfigStream& configFile);

	void initializeVimba();
	void validateCamera(const CameraPtrVector& Cameras);

	void releaseBuffer();

	void checkDisplayInterval();

	bool isStreamingAvailable();

	void prepareCapture();

	void startCapture();

	void stopCapture();

	std::array<unsigned, 4> getROIIncrement(); // xwidth, ywidth, offsetx, offsety

	void setROI(int width, int height, int offsetx, int offsety);

	void resetFullROI();

	void updateCurrentSettings();

	void setExpActive(bool active);

	void setPicsPerRep(int picsperrep);

	//void setCurrentRepVarNumber(size_t rep, size_t var);
	std::pair<size_t, size_t> getCurrentRepVarNumber(unsigned int currentPicNumber);

	std::string CameraName() { return cameraName; }
	MakoSettingControl& getMakoCtrl() { return makoCtrl; }
	SP_DECL(FrameObserver) getFrameObs() { return frameObs; }
	CameraPtr& getCameraPtr() { return cameraPtr; }
	MakoSettings getRunningSettings() { return runSettings; }
	std::pair<unsigned, unsigned> getTriggerLine() { return camInfo.triggerLine; }

signals:
	void makoFinished();
	void makoStarted();

public:
	const unsigned int BUFFER_COUNT = 7;

private:
	VimbaSystem& m_VimbaSystem;
	//remember to setParent in the gui class
	MakoSettingControl makoCtrl;
	SP_DECL(FrameObserver) frameObs;
	CameraInfo camInfo;

	CameraPtr cameraPtr;
	std::string cameraName; // name shown on top of viewer with camID, not same as name in CameraInfo
	QtCameraObserverPtr pDeviceObs; // For monitoring device list change
	// official copy.
	MakoSettings runSettings;
	MakoSettings expRunSettings;
    //only for prepare experiment, do want the start of camera (if not) happens after setting all parameters and before exp start
	//MakoCamera* cam = nullptr; // does not work since call start cam from expThread is not ok due to the gui stuff that has to be manipulated in the main thread

};

