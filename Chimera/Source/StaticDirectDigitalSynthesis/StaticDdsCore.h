#pragma once
#include <GeneralObjects/IDeviceCore.h>
#include <StaticDirectDigitalSynthesis/StaticDdsStructures.h>
#include <StaticDirectDigitalSynthesis/StaticDDSFlume.h>
#include <ParameterSystem/ParameterSystemStructures.h>

class ConfigStream;
class ExpThreadWorker;
class DataLogger;
class StaticDdsCore : public IDeviceCore
{
public:
    // THIS CLASS IS NOT COPYABLE.
    StaticDdsCore(const StaticDdsCore&) = delete;
    StaticDdsCore& operator=(const StaticDdsCore&) = delete;

    StaticDdsCore(bool safemode, const std::string& port, unsigned baudrate);

    virtual void loadExpSettings(ConfigStream& stream) override;
    virtual void logSettings(DataLogger& logger, ExpThreadWorker* threadworker) override;
    virtual void calculateVariations(std::vector<parameterType>& params, ExpThreadWorker* threadworker) override;
    virtual void programVariation(unsigned variation, std::vector<parameterType>& params,
        ExpThreadWorker* threadworker) override;
    virtual void normalFinish() override {};
    virtual void errorFinish() override {};
    virtual std::string getDelim() override { return configDelim; };

    StaticDDSSettings getSettingsFromConfig(ConfigStream& file);

    std::string getDeviceInfo();
    void setStaticDDSExpSetting(StaticDDSSettings tmpSetting); // used only for ProgramNow in StaticAOSystem

    const std::string configDelim = "STATIC_DDS_SYSTEM";
    const bool safemode;
private:
    std::string getDDSCommand(double ddsfreqVal);
    void writeDDSs(std::array<double, size_t(StaticDDSGrid::total)> outputs);
    bool checkBound(double ddsfreqVal);

public:
    static constexpr double ddsResolutionInst = 1e-6; // 1 Hz
    const int numFreqDigits = static_cast<int>(abs(round(log10(ddsResolutionInst) - 0.49)));
    const double minVal = 0.1;
    const double maxVal = 1000;

private:
    StaticDDSFlume sddsFlume;
    StaticDDSSettings expSettings;    
};

