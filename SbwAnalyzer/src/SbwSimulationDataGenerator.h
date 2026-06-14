#ifndef SBW_SIMULATION_DATA_GENERATOR
#define SBW_SIMULATION_DATA_GENERATOR

#include <AnalyzerHelpers.h>

class SbwAnalyzerSettings;


class SbwSimulationDataGenerator
{
public:
	SbwSimulationDataGenerator();
	~SbwSimulationDataGenerator();

	void Initialize(U32 simulation_sample_rate, SbwAnalyzerSettings* settings);
	U32 GenerateSimulationData(U64 newest_sample_requested, U32 sample_rate, SimulationChannelDescriptor** simulation_channels);

protected:
	void CreateJtagTransaction();
	void MoveState(const char* tms);
	void Scan(U32 in, U32 out, U32 bits);

	SbwAnalyzerSettings* m_pSettings;
	U32 m_SimulationSampleRateHz;

	ClockGenerator m_ClockGenerator;

	SimulationChannelDescriptorGroup m_JtagSimulationChannels;
	SimulationChannelDescriptor* m_pTCK;
	SimulationChannelDescriptor* m_pTMS;
	SimulationChannelDescriptor* m_pTDI;
	SimulationChannelDescriptor* m_pTDO;
};


#endif //SBW_SIMULATION_DATA_GENERATOR
