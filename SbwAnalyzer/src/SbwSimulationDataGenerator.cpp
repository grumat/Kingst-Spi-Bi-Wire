#include <cstdlib>

#include "SbwSimulationDataGenerator.h"
#include "SbwAnalyzerSettings.h"


SbwSimulationDataGenerator::SbwSimulationDataGenerator()
{
}


SbwSimulationDataGenerator::~SbwSimulationDataGenerator()
{
}


void SbwSimulationDataGenerator::Initialize(U32 simulation_sample_rate, SbwAnalyzerSettings* settings)
{
	m_SimulationSampleRateHz = simulation_sample_rate;
	m_pSettings = settings;

	m_ClockGenerator.Init(simulation_sample_rate / 10, simulation_sample_rate);

	m_pTCK = m_JtagSimulationChannels.Add(settings->m_TCKChannel, m_SimulationSampleRateHz, BIT_LOW);
	m_pTMS = m_JtagSimulationChannels.Add(settings->m_TDIOChannel, m_SimulationSampleRateHz, BIT_LOW);

	m_JtagSimulationChannels.AdvanceAll(m_ClockGenerator.AdvanceByHalfPeriod(10.0));
}


U32 SbwSimulationDataGenerator::GenerateSimulationData(U64 largest_sample_requested, U32 sample_rate, SimulationChannelDescriptor** simulation_channels)
{
	U64 adjusted_largest_sample_requested = AnalyzerHelpers::AdjustSimulationTargetSample(largest_sample_requested, sample_rate, m_SimulationSampleRateHz);

	while (m_pTCK->GetCurrentSampleNumber() < adjusted_largest_sample_requested)
	{
		MoveState("1111100");
		Scan(0x83, 0x89, 8);
		MoveState("1111100");
		Scan(0x2401, 0, 16);
		MoveState("1111100");
		Scan(0x0000, 0, 16);
		MoveState("1111");

		m_JtagSimulationChannels.AdvanceAll(m_ClockGenerator.AdvanceByHalfPeriod((rand() % 10) * 10.0));
	}

	*simulation_channels = m_JtagSimulationChannels.GetArray();
	return m_JtagSimulationChannels.GetCount();
}


void SbwSimulationDataGenerator::CreateJtagTransaction()
{
	switch (rand() % 2)
	{
	case 0:
		MoveState("0100");
		Scan(rand(), rand(), rand() % 24 + 1);
		MoveState("1111");
		break;

	case 1:
		MoveState("0110");
		Scan(rand(), rand(), rand() % 24 + 1);
		MoveState("1111");
		break;
	}
}


void SbwSimulationDataGenerator::MoveState(const char* tms)
{
	while (*tms)
	{
		m_pTMS->TransitionIfNeeded(static_cast<BitState>(*tms++ == '1'));

		m_JtagSimulationChannels.AdvanceAll(m_ClockGenerator.AdvanceByHalfPeriod(0.5));
		m_pTCK->Transition();

		m_JtagSimulationChannels.AdvanceAll(m_ClockGenerator.AdvanceByHalfPeriod(0.5));
		m_pTCK->Transition();
	}
}


void SbwSimulationDataGenerator::Scan(U32 in, U32 out, U32 bits)
{
	while (bits--)
	{
		m_pTMS->TransitionIfNeeded(static_cast<BitState>(in & 1));
		in >>= 1;

		if (bits == 0)
		{
			m_pTMS->Transition();
		}

		m_JtagSimulationChannels.AdvanceAll(m_ClockGenerator.AdvanceByHalfPeriod(0.5));
		m_pTCK->Transition();

		m_JtagSimulationChannels.AdvanceAll(m_ClockGenerator.AdvanceByHalfPeriod(0.5));
		m_pTCK->Transition();
	}
}

