#ifndef SBW_ANALYZER_H
#define SBW_ANALYZER_H

#include <Analyzer.h>
#include "SbwAnalyzerResults.h"
#include "SbwSimulationDataGenerator.h"


enum class SbwState : U32
{
    kSbwTms,
    kSbwTdi,
    kSbwTdo,
    kSbwIdle
};


enum class JtagState : U32
{
    kJtagReset,
    kJtagIdle,

    kJtagSelectDR,
    kJtagCaptureDR,
    kJtagShiftDR,
    kJtagExit1DR,
    kJtagPauseDR,
    kJtagExit2DR,
    kJtagUpdateDR,

    kJtagSelectIR,
    kJtagCaptureIR,
    kJtagShiftIR,
    kJtagExit1IR,
    kJtagPauseIR,
    kJtagExit2IR,
    kJtagUpdateIR
};


class SbwAnalyzerSettings;


class SbwAnalyzer : public Analyzer
{
public:
    SbwAnalyzer();
    virtual ~SbwAnalyzer();
    virtual void SetupResults();
    virtual void WorkerThread();

    virtual U32 GenerateSimulationData(U64 newest_sample_requested, U32 sample_rate, SimulationChannelDescriptor **simulation_channels);
    virtual U32 GetMinimumSampleRateHz();

    virtual const char *GetAnalyzerVersion() const;
    virtual const char *GetAnalyzerName() const;
    virtual bool NeedsRerun();

protected:
    void Setup();
    void ProcessJtag();
    void ProcessStep();
    void CommitFrame(U64 ending_sample);

protected:
    std::auto_ptr< SbwAnalyzerSettings > m_ptrSettings;
    std::auto_ptr< SbwAnalyzerResults > m_ptrResults;
    bool m_fSimulationInitilized;
    SbwSimulationDataGenerator m_SimulationDataGenerator;

    AnalyzerChannelData *m_pTCK;
    AnalyzerChannelData *m_pTDIO;

    U64 m_CurrentSample, m_FirstSample, m_LastRisingSample, m_TCKTimeout, m_TDOSkip;

    enum SbwState m_Slot;
    enum JtagState m_State;
    bool m_fTMSValue;
    U64 m_DataIn, m_DataOut;
    U32 m_Bits;

    bool m_fFramePending;
    U64 m_PendingStartSample, m_PendingData1, m_PendingData2;
    U32 m_PendingFlags;
    U8 m_PendingType;
    JtagState m_PendingNextState;

};


extern "C" ANALYZER_EXPORT const char *__cdecl GetAnalyzerName();
extern "C" ANALYZER_EXPORT Analyzer *__cdecl CreateAnalyzer();
extern "C" ANALYZER_EXPORT void __cdecl DestroyAnalyzer(Analyzer *analyzer);


#endif //SBW_ANALYZER_H
