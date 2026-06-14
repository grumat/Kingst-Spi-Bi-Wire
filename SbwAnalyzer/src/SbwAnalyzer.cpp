#include "SbwAnalyzer.h"
#include "SbwAnalyzerSettings.h"
#include <AnalyzerChannelData.h>


SbwAnalyzer::SbwAnalyzer()
	: Analyzer()
	, m_ptrSettings(new SbwAnalyzerSettings())
	, m_fSimulationInitilized(false)
	, m_pTCK(NULL)
	, m_pTDIO(NULL)
	, m_fFramePending(false)
	, m_LastRisingSample(0)
{
	SetAnalyzerSettings(m_ptrSettings.get());
}


SbwAnalyzer::~SbwAnalyzer()
{
	KillThread();
}


void SbwAnalyzer::SetupResults()
{
	m_ptrResults.reset(new SbwAnalyzerResults(this, m_ptrSettings.get()));
	SetAnalyzerResults(m_ptrResults.get());

	m_ptrResults->AddChannelBubblesWillAppearOn(m_ptrSettings->m_TCKChannel);
	m_ptrResults->AddChannelBubblesWillAppearOn(m_ptrSettings->m_TDIOChannel);
}


void SbwAnalyzer::WorkerThread()
{
	Setup();

	m_Slot = SbwState::kSbwTms;
	m_State = JtagState::kJtagReset;
	m_DataIn = m_DataOut = 0;
	m_Bits = 0;
	m_fFramePending = false;

	if (m_pTCK->GetBitState() == BIT_LOW)
		m_pTCK->AdvanceToNextEdge();

	m_FirstSample = m_pTCK->GetSampleNumber();
	m_LastRisingSample = m_FirstSample;

	for (; ;)
	{
		m_pTCK->AdvanceToNextEdge();
		m_CurrentSample = m_pTCK->GetSampleNumber();

		ProcessStep();

		m_pTCK->AdvanceToNextEdge();
		U64 rising_sample = m_pTCK->GetSampleNumber();
		m_LastRisingSample = rising_sample;

		if (m_fFramePending)
			CommitFrame(rising_sample);

		if (rising_sample - m_CurrentSample > m_TCKTimeout)
		{
			m_ptrResults->AddMarker(rising_sample, AnalyzerResults::Stop, m_ptrSettings->m_TCKChannel);
			m_Slot = SbwState::kSbwIdle;
			m_State = JtagState::kJtagReset;
		}

		CheckIfThreadShouldExit();
	}
}


void SbwAnalyzer::Setup()
{
	m_pTCK = GetAnalyzerChannelData(m_ptrSettings->m_TCKChannel);
	m_pTDIO = GetAnalyzerChannelData(m_ptrSettings->m_TDIOChannel);

	m_TCKTimeout = GetSampleRate() / 14286; // 7 us
	m_TDOSkip = GetSampleRate() / 10000000; // 100 ns
}


void SbwAnalyzer::ProcessJtag()
{
	enum JtagState next_state = m_State;

	switch (m_State)
	{
	case JtagState::kJtagReset:
		next_state = m_fTMSValue ? JtagState::kJtagReset : JtagState::kJtagIdle;
		break;

	case JtagState::kJtagIdle:
		next_state = m_fTMSValue ? JtagState::kJtagSelectDR : JtagState::kJtagIdle;
		break;

	case JtagState::kJtagSelectDR:
		next_state = m_fTMSValue ? JtagState::kJtagSelectIR : JtagState::kJtagCaptureDR;
		break;

	case JtagState::kJtagCaptureDR:
		next_state = m_fTMSValue ? JtagState::kJtagExit1DR : JtagState::kJtagShiftDR;
		break;

	case JtagState::kJtagShiftDR:
		next_state = m_fTMSValue ? JtagState::kJtagExit1DR : JtagState::kJtagShiftDR;
		break;

	case JtagState::kJtagExit1DR:
		next_state = m_fTMSValue ? JtagState::kJtagUpdateDR : JtagState::kJtagPauseDR;
		break;

	case JtagState::kJtagPauseDR:
		next_state = m_fTMSValue ? JtagState::kJtagExit2DR : JtagState::kJtagPauseDR;
		break;

	case JtagState::kJtagExit2DR:
		next_state = m_fTMSValue ? JtagState::kJtagUpdateDR : JtagState::kJtagShiftDR;
		break;

	case JtagState::kJtagUpdateDR:
		next_state = m_fTMSValue ? JtagState::kJtagSelectDR : JtagState::kJtagIdle;
		break;

	case JtagState::kJtagSelectIR:
		next_state = m_fTMSValue ? JtagState::kJtagReset : JtagState::kJtagCaptureIR;
		break;

	case JtagState::kJtagCaptureIR:
		next_state = m_fTMSValue ? JtagState::kJtagExit1IR : JtagState::kJtagShiftIR;
		break;

	case JtagState::kJtagShiftIR:
		next_state = m_fTMSValue ? JtagState::kJtagExit1IR : JtagState::kJtagShiftIR;
		break;

	case JtagState::kJtagExit1IR:
		next_state = m_fTMSValue ? JtagState::kJtagUpdateIR : JtagState::kJtagPauseIR;
		break;

	case JtagState::kJtagPauseIR:
		next_state = m_fTMSValue ? JtagState::kJtagExit2IR : JtagState::kJtagPauseIR;
		break;

	case JtagState::kJtagExit2IR:
		next_state = m_fTMSValue ? JtagState::kJtagUpdateIR : JtagState::kJtagShiftIR;
		break;

	case JtagState::kJtagUpdateIR:
		next_state = m_fTMSValue ? JtagState::kJtagSelectDR : JtagState::kJtagIdle;
		break;
	}

	if (next_state != m_State)
	{
		m_fFramePending = true;
		m_PendingStartSample = m_FirstSample;
		m_PendingData1 = m_DataIn;
		m_PendingData2 = m_DataOut;
		m_PendingFlags = static_cast<U32>(m_State);
		m_PendingType = (U8)m_Bits;
		m_PendingNextState = next_state;

		m_DataIn = m_DataOut = 0;
		m_Bits = 0;
	}
}


void SbwAnalyzer::CommitFrame(U64 ending_sample)
{
	m_fFramePending = false;

	Frame result_frame;
	result_frame.mStartingSampleInclusive = m_PendingStartSample;
	result_frame.mEndingSampleInclusive = ending_sample;
	result_frame.mData1 = m_PendingData1;
	result_frame.mData2 = m_PendingData2;
	result_frame.mFlags = m_PendingFlags;
	result_frame.mType = m_PendingType;

	m_ptrResults->AddFrame(result_frame);
	m_ptrResults->CommitResults();

	m_FirstSample = ending_sample;
	m_State = m_PendingNextState;
}


void SbwAnalyzer::ProcessStep()
{
	m_pTDIO->AdvanceToAbsPosition(m_CurrentSample);

	m_ptrResults->AddMarker(m_CurrentSample, AnalyzerResults::DownArrow, m_ptrSettings->m_TCKChannel);

	switch (m_Slot)
	{
	case SbwState::kSbwIdle:
		m_Slot = SbwState::kSbwTms;
		break;

	case SbwState::kSbwTms:
		m_Slot = SbwState::kSbwTdi;
		m_fTMSValue = m_pTDIO->GetBitState();
		break;

	case SbwState::kSbwTdi:
		m_Slot = SbwState::kSbwTdo;
		if (m_State == JtagState::kJtagShiftDR || m_State == JtagState::kJtagShiftIR)
		{
			m_DataIn = (m_DataIn << 1) | (m_pTDIO->GetBitState() ? 1 : 0);
			m_ptrResults->AddMarker(m_CurrentSample, AnalyzerResults::Dot, m_ptrSettings->m_TDIOChannel);
		}
		break;

	case SbwState::kSbwTdo:
		m_Slot = SbwState::kSbwTms;
		if (m_State == JtagState::kJtagShiftDR || m_State == JtagState::kJtagShiftIR)
		{
			m_pTDIO->Advance((U32)m_TDOSkip);
			m_DataOut = (m_DataOut << 1) | (m_pTDIO->GetBitState() ? 1 : 0);
			m_Bits++;

			U64 dot_pos = m_CurrentSample + (m_CurrentSample - m_LastRisingSample) / 2;
			AnalyzerResults::MarkerType tdo_marker = m_pTDIO->GetBitState() == BIT_HIGH ? AnalyzerResults::One : AnalyzerResults::Zero;
			m_ptrResults->AddMarker(dot_pos, tdo_marker, m_ptrSettings->m_TDIOChannel);
		}
		ProcessJtag();
	}
}


bool SbwAnalyzer::NeedsRerun()
{
	return false;
}


U32 SbwAnalyzer::GenerateSimulationData(U64 minimum_sample_index, U32 device_sample_rate, SimulationChannelDescriptor** simulation_channels)
{
	if (m_fSimulationInitilized == false)
	{
		m_SimulationDataGenerator.Initialize(GetSimulationSampleRate(), m_ptrSettings.get());
		m_fSimulationInitilized = true;
	}

	return m_SimulationDataGenerator.GenerateSimulationData(minimum_sample_index, device_sample_rate, simulation_channels);
}


U32 SbwAnalyzer::GetMinimumSampleRateHz()
{
	return 10000;
}


const char* SbwAnalyzer::GetAnalyzerVersion() const
{
	return "2.0";
}


const char* SbwAnalyzer::GetAnalyzerName() const
{
	return "Spy-Bi-Wire";
}


const char* GetAnalyzerName()
{
	return "Spy-Bi-Wire";
}


Analyzer* CreateAnalyzer()
{
	return new SbwAnalyzer();
}


void DestroyAnalyzer(Analyzer* analyzer)
{
	delete analyzer;
}
