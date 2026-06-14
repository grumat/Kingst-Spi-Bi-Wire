#include "SbwAnalyzerSettings.h"

#include <AnalyzerHelpers.h>
#include <sstream>
#include <cstring>


SbwAnalyzerSettings::SbwAnalyzerSettings()
	: m_TDIOChannel(UNDEFINED_CHANNEL)
	, m_TCKChannel(UNDEFINED_CHANNEL)
{
	m_ptrTCKChannelInterface.reset(new AnalyzerSettingInterfaceChannel());
	m_ptrTCKChannelInterface->SetTitleAndTooltip("SBWTCK", "Test Clock");
	m_ptrTCKChannelInterface->SetChannel(m_TCKChannel);

	m_ptrTDIOChannelInterface.reset(new AnalyzerSettingInterfaceChannel());
	m_ptrTDIOChannelInterface->SetTitleAndTooltip("SBWTDIO", "Test Data Input/Output");
	m_ptrTDIOChannelInterface->SetChannel(m_TDIOChannel);

	AddInterface(m_ptrTCKChannelInterface.get());
	AddInterface(m_ptrTDIOChannelInterface.get());

	AddExportOption(0, "Export as text/csv file");
	AddExportExtension(0, "text", "txt");
	AddExportExtension(0, "csv", "csv");

	ClearChannels();
	AddChannel(m_TCKChannel, "SBWTCK", false);
	AddChannel(m_TDIOChannel, "SBWTDIO", false);
}


SbwAnalyzerSettings::~SbwAnalyzerSettings()
{
}


bool SbwAnalyzerSettings::SetSettingsFromInterfaces()
{
	Channel tck = m_ptrTCKChannelInterface->GetChannel();
	Channel tdio = m_ptrTDIOChannelInterface->GetChannel();

	std::vector<Channel> channels;
	channels.push_back(tck);
	channels.push_back(tdio);

	if (AnalyzerHelpers::DoChannelsOverlap(&channels[0], (U32)channels.size()) == true)
	{
		SetErrorText("Please select different channels for each input.");
		return false;
	}

	m_TCKChannel = m_ptrTCKChannelInterface->GetChannel();
	m_TDIOChannel = m_ptrTDIOChannelInterface->GetChannel();

	ClearChannels();
	AddChannel(m_TCKChannel, "SBWTCK", m_TCKChannel != UNDEFINED_CHANNEL);
	AddChannel(m_TDIOChannel, "SBWTDIO", m_TDIOChannel != UNDEFINED_CHANNEL);

	return true;
}


void SbwAnalyzerSettings::LoadSettings(const char* settings)
{
	SimpleArchive text_archive;
	text_archive.SetString(settings);

	const char* name_string;
	text_archive >> &name_string;
	if (strcmp(name_string, "SbwAnalyzer") != 0)
	{
		AnalyzerHelpers::Assert("SbwAnalyzer: Provided with a settings string that doesn't belong to us;");
	}

	text_archive >> m_TCKChannel;
	text_archive >> m_TDIOChannel;

	ClearChannels();
	AddChannel(m_TCKChannel, "SBWTCK", m_TCKChannel != UNDEFINED_CHANNEL);
	AddChannel(m_TDIOChannel, "SBWTDIO", m_TDIOChannel != UNDEFINED_CHANNEL);

	UpdateInterfacesFromSettings();
}


const char* SbwAnalyzerSettings::SaveSettings()
{
	SimpleArchive text_archive;

	text_archive << "SbwAnalyzer";
	text_archive << m_TCKChannel;
	text_archive << m_TDIOChannel;

	return SetReturnString(text_archive.GetString());
}


void SbwAnalyzerSettings::UpdateInterfacesFromSettings()
{
	m_ptrTCKChannelInterface->SetChannel(m_TCKChannel);
	m_ptrTDIOChannelInterface->SetChannel(m_TDIOChannel);
}
