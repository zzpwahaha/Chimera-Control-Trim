#pragma once
#include <GeneralObjects/IDeviceCore.h>
#include <StaticAnalogOutput/StaticAoStructures.h>
#include <GeneralFlumes/BoostSyncUDP.h>
#include <ParameterSystem/ParameterSystemStructures.h>

class ConfigStream;
class ExpThreadWorker;
class DataLogger;
class StaticAoCore : public IDeviceCore
{
public:
    // THIS CLASS IS NOT COPYABLE.
    StaticAoCore(const StaticAoCore&) = delete;
    StaticAoCore& operator=(const StaticAoCore&) = delete;

    StaticAoCore(bool safemode, const std::string& host, int port);
    
    virtual void loadExpSettings(ConfigStream& stream) override;
    virtual void logSettings(DataLogger& logger, ExpThreadWorker* threadworker) override;
    virtual void calculateVariations(std::vector<parameterType>& params, ExpThreadWorker* threadworker) override;
    virtual void programVariation(unsigned variation, std::vector<parameterType>& params,
        ExpThreadWorker* threadworker) override;
    virtual void normalFinish() override {};
    virtual void errorFinish() override {};
    virtual std::string getDelim() override { return configDelim; };

    StaticAOSettings getSettingsFromConfig(ConfigStream& file);
    
    std::string getDeviceInfo();
    void setStaticAOExpSetting(StaticAOSettings tmpSetting); // used only for ProgramNow in StaticAOSystem

    const std::string configDelim = "STATIC_DAC_SYSTEM";
    const bool safemode;
private:
    std::vector<unsigned char> getDACbytes(double dacVal);
    void writeDACs(std::array<double, size_t(StaticAOGrid::total)> outputs);
    bool checkBound(double dacVal);

public:
    static constexpr double dacResolutionInst = 20.0 / 0xfffff; /*20bit dac*/
    const int numDigitsInst = static_cast<int>(abs(round(log10(dacResolutionInst) - 0.49)));
    const double minVal = -10.0;
    const double maxVal = 10.0;

private:
    BoostSyncUDP fpga;
    StaticAOSettings expSettings;

};

