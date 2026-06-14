#ifndef SBW_ANALYZER_SETTINGS
#define SBW_ANALYZER_SETTINGS

#include <AnalyzerSettings.h>
#include <AnalyzerTypes.h>


class SbwAnalyzerSettings : public AnalyzerSettings
{
public:
    SbwAnalyzerSettings();
    virtual ~SbwAnalyzerSettings();

    virtual bool SetSettingsFromInterfaces();
    virtual void LoadSettings(const char *settings);
    virtual const char *SaveSettings();

    void UpdateInterfacesFromSettings();

    Channel m_TDIOChannel;
    Channel m_TCKChannel;

protected:
    std::auto_ptr< AnalyzerSettingInterfaceChannel > m_ptrTDIOChannelInterface;
    std::auto_ptr< AnalyzerSettingInterfaceChannel > m_ptrTCKChannelInterface;
};


#endif //SBW_ANALYZER_SETTINGS
