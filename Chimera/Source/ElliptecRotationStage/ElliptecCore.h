#pragma once
#include <GeneralObjects/IDeviceCore.h>
#include <ParameterSystem/ParameterSystemStructures.h>
#include <ElliptecRotationStage/ElliptecSettings.h>
#include <ElliptecRotationStage/ElliptecFlume.h>

class ConfigStream;
class ExpThreadWorker;
class DataLogger;
class ElliptecCore : public IDeviceCore
{
    Q_OBJECT
public:
    // THIS CLASS IS NOT COPYABLE.
    ElliptecCore(const ElliptecCore&) = delete;
    ElliptecCore& operator=(const ElliptecCore&) = delete;
    ElliptecCore(bool safemode, const std::string& port);

    virtual void loadExpSettings(ConfigStream& stream) override;
    virtual void logSettings(DataLogger& logger, ExpThreadWorker* threadworker) override;
    virtual void calculateVariations(std::vector<parameterType>& params, ExpThreadWorker* threadworker) override;
    virtual void programVariation(unsigned variation, std::vector<parameterType>& params,
        ExpThreadWorker* threadworker) override;
    virtual void normalFinish() override {};
    virtual void errorFinish() override {};
    virtual std::string getDelim() override { return configDelim; };

    ElliptecSettings getSettingsFromConfig(ConfigStream& file);

    std::string getDeviceInfo();
    void setElliptecExpSettings(ElliptecSettings tmpSetting); // used only for ProgramNow in ElliptecSystem
    double getElliptecPosition(unsigned channel);

    const std::string configDelim = "ELLIPTEC";
    const bool safemode;

signals:
    void currentAngle(unsigned id, double angle);

private:
    double normalizeToRange(double angle, double min, double max);
    std::string getElliptecCommand(unsigned channel, double angle);
    void writeElliptecs(std::array<double, size_t(ElliptecGrid::total)> outputs);
    void writeElliptec(unsigned channel, double angle);
    bool checkBound(double angle);

public:
    static constexpr double elliptecResolutionInst = 360.0/0x23000; // 360 degree / 0x23000 counts/rev
    const int numAngleDigits = static_cast<int>(abs(round(log10(elliptecResolutionInst) - 0.49)));
    // bound for Chimera 
    const double maxVal = 360.0;
    const double minVal = -360.0;

private:
    ElliptecFlume ellFlume;
    ElliptecSettings expSettings;

};

