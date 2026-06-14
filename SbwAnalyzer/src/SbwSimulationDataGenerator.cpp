#include <cstdlib>

#include "SbwSimulationDataGenerator.h"
#include "SbwAnalyzerSettings.h"

SbwSimulationDataGenerator::SbwSimulationDataGenerator()
{
}

SbwSimulationDataGenerator::~SbwSimulationDataGenerator()
{
}

void SbwSimulationDataGenerator::Initialize(U32 simulation_sample_rate, SbwAnalyzerSettings *settings)
{
    mSimulationSampleRateHz = simulation_sample_rate;
    mSettings = settings;

    mClockGenerator.Init(simulation_sample_rate / 10, simulation_sample_rate);

    mTCK = mJtagSimulationChannels.Add(settings->mTCKChannel, mSimulationSampleRateHz, BIT_LOW);
    mTMS = mJtagSimulationChannels.Add(settings->mTDIOChannel, mSimulationSampleRateHz, BIT_LOW);

    mJtagSimulationChannels.AdvanceAll(mClockGenerator.AdvanceByHalfPeriod(10.0));
}

U32 SbwSimulationDataGenerator::GenerateSimulationData(U64 largest_sample_requested, U32 sample_rate, SimulationChannelDescriptor **simulation_channels)
{
    U64 adjusted_largest_sample_requested = AnalyzerHelpers::AdjustSimulationTargetSample(largest_sample_requested, sample_rate, mSimulationSampleRateHz);

    while (mTCK->GetCurrentSampleNumber() < adjusted_largest_sample_requested) {
        MoveState("1111100");
        Scan(0x83, 0x89, 8);
        MoveState("1111100");
        Scan(0x2401, 0, 16);
        MoveState("1111100");
        Scan(0x0000, 0, 16);
        MoveState("1111");

        mJtagSimulationChannels.AdvanceAll(mClockGenerator.AdvanceByHalfPeriod((rand() % 10) * 10.0));
    }

    *simulation_channels = mJtagSimulationChannels.GetArray();
    return mJtagSimulationChannels.GetCount();
}

void SbwSimulationDataGenerator::CreateJtagTransaction()
{
    switch (rand() % 2) {
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

void SbwSimulationDataGenerator::MoveState(const char *tms)
{
    while (*tms) {
        mTMS->TransitionIfNeeded(static_cast<BitState>(*tms++ == '1'));

        mJtagSimulationChannels.AdvanceAll(mClockGenerator.AdvanceByHalfPeriod(0.5));
        mTCK->Transition();

        mJtagSimulationChannels.AdvanceAll(mClockGenerator.AdvanceByHalfPeriod(0.5));
        mTCK->Transition();
    }
}

void SbwSimulationDataGenerator::Scan(U32 in, U32 out, U32 bits)
{
    while (bits--) {
        mTMS->TransitionIfNeeded(static_cast<BitState>(in & 1));
        in >>= 1;

        if (bits == 0) {
            mTMS->Transition();
        }

        mJtagSimulationChannels.AdvanceAll(mClockGenerator.AdvanceByHalfPeriod(0.5));
        mTCK->Transition();

        mJtagSimulationChannels.AdvanceAll(mClockGenerator.AdvanceByHalfPeriod(0.5));
        mTCK->Transition();
    }
}
