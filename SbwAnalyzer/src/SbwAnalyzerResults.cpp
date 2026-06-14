#include "SbwAnalyzerResults.h"
#include <AnalyzerHelpers.h>
#include "SbwAnalyzer.h"
#include "SbwAnalyzerSettings.h"
#include <iostream>
#include <sstream>
#include <cstdio>

#pragma warning(disable: 4996) //warning C4996: 'sprintf': This function or variable may be unsafe. Consider using sprintf_s instead.

static const char *JtagStateStr[] = {
    "Reset",
    "Run-Test/IDLE",

    "Select DR-Scan",
    "Capture DR",
    "Shift-DR",
    "Exit1-DR",
    "Pause-DR",
    "Exit2-DR",
    "Update-DR",

    "Select IR-Scan",
    "Capture-IR",
    "Shift-IR",
    "Exit1-IR",
    "Pause-IR",
    "Exit2-IR",
    "Update-IR"
};

static const struct {
    unsigned char ir;
    const char *name;
} MSPIRValues[] = {
    {0x11, "IR_JMB_WRITE_32BIT_MODE"},
    {0x21, "IR_ADDR_CAPTURE"},
    {0x22, "IR_DATA_PSA"},
    {0x24, "IR_EX_BLOW"},
    {0x28, "IR_CNTRL_SIG_CAPTURE"},
    {0x30, "IR_EMEX_WRITE_CONTROL"},
    {0x38, "IR_FLASH_UPDATE"},
    {0x41, "IR_ADDR_LOW_BYTE"},
    {0x42, "IR_DATA_CAPTURE"},
    {0x44, "IR_PREPARE_BLOW"},
    {0x46, "IR_JSTATE_ID"},
    {0x48, "IR_CNTRL_SIG_LOW_BYTE"},
    {0x50, "IR_EMEX_READ_TRIGGER"},
    {0x54, "IR_TEST_REG"},
    {0x58, "IR_FLASH_CAPTURE"},
    {0x61, "IR_CAPTURE_CPU_REG"},
    {0x62, "IR_SHIFT_OUT_PSA"},
    {0x81, "IR_ADDR_HIGH_BYTE"},
    {0x82, "IR_DATA_16BIT"},
    {0x86, "IR_JMB_EXCHANGE"},
    {0x88, "IR_CNTRL_SIG_HIGH_BYTE"},
    {0x90, "IR_EMEX_DATA_EXCHANGE"},
    {0x94, "IR_CONFIG_FUSES"},
    {0x98, "IR_FLASH_16BIT_UPDATE"},
    {0xA1, "IR_DATA_TO_ADDR"},
    {0xA2, "IR_DATA_16BIT_OPT"},
    {0xA8, "IR_CNTRL_SIG_RELEASE"},
    {0xB0, "IR_EMEX_DATA_EXCHANGE32"},
    {0xC1, "IR_ADDR_16BIT"},
    {0xC2, "IR_DATA_QUICK"},
    {0xC8, "IR_CNTRL_SIG_16BIT"},
    {0xD0, "IR_EMEX_READ_CONTROL"},
    {0xD8, "IR_FLASH_16BIT_IN"},
    {0xE1, "IR_DEVICE_ID"},
    {0xE2, "IR_DTA"},
    {0xE8, "IR_CORE_IP_ID"},
    {0xF4, "IR_TEST_3VREG"},
    {0xFF, "IR_BYPASS"},
    {0, NULL}
};

SbwAnalyzerResults::SbwAnalyzerResults(SbwAnalyzer *analyzer, SbwAnalyzerSettings *settings)
    : AnalyzerResults(),
      mSettings(settings),
      mAnalyzer(analyzer)
{
}

SbwAnalyzerResults::~SbwAnalyzerResults()
{
}

void SbwAnalyzerResults::GenerateBubbleText(U64 frame_index, Channel& channel, DisplayBase display_base)
{
	char tdi[32], tdo[32], buf[128];

	ClearResultStrings();
	Frame frame = GetFrame(frame_index);

	if (channel == mSettings->mTCKChannel)
    {
		AddResultString(JtagStateStr[frame.mFlags & 0x0f]);
	}
	else if ((frame.mFlags & 0xf) == JtagShiftDR)
    {
		if (channel == mSettings->mTDIOChannel)
        {
			AnalyzerHelpers::GetNumberString(frame.mData1, display_base, frame.mType, tdi, _countof(tdi));
			AnalyzerHelpers::GetNumberString(frame.mData2, display_base, frame.mType, tdo, _countof(tdo));
			sprintf(buf, "%s / %s", tdi, tdo);
			AddResultString(buf);
		}
	}
	else if ((frame.mFlags & 0xf) == JtagShiftIR)
    {
		int idx;
		for (idx = 0; MSPIRValues[idx].name; idx++)
        {
			if (MSPIRValues[idx].ir == frame.mData1)
            {
                AnalyzerHelpers::GetNumberString(frame.mData2, display_base, frame.mType, tdo, _countof(tdo));
				sprintf(buf, "%s / %s", MSPIRValues[idx].name, tdo);
                AddResultString(buf);
				return;
			}
		}

		AnalyzerHelpers::GetNumberString(frame.mData1, display_base, frame.mType, tdi, _countof(tdi));
		AnalyzerHelpers::GetNumberString(frame.mData2, display_base, frame.mType, tdo, _countof(tdo));
		sprintf(buf, "%s / %s", tdi, tdo);
		AddResultString(buf);
	}
}

void SbwAnalyzerResults::GenerateExportFile(const char *file, DisplayBase display_base, U32 /*export_type_user_id*/)
{
    std::stringstream ss;
    void *f = AnalyzerHelpers::StartFile(file);

    U64 trigger_sample = mAnalyzer->GetTriggerSample();
    U32 sample_rate = mAnalyzer->GetSampleRate();

    ss << "Time [s],Packet ID,State,TDI,TDO" << std::endl;

    U64 num_frames = GetNumFrames();
    for (U32 i = 0; i < num_frames; i++) {
        Frame frame = GetFrame(i);

        if ((frame.mFlags & 0xf) != JtagShiftDR && (frame.mFlags & 0xf) != JtagShiftIR) {
            continue;
        }

        char time_str[128], tdi_str[128], tdo_str[128];

        AnalyzerHelpers::GetTimeString(frame.mStartingSampleInclusive, trigger_sample, sample_rate, time_str, 128);

        AnalyzerHelpers::GetNumberString(frame.mData1, display_base, frame.mType, tdi_str, _countof(tdi_str));
        AnalyzerHelpers::GetNumberString(frame.mData2, display_base, frame.mType, tdo_str, _countof(tdo_str));

        U64 packet_id = GetPacketContainingFrameSequential(i);
        if (packet_id != INVALID_RESULT_INDEX) {
            ss << time_str << "," << packet_id << "," << JtagStateStr[frame.mFlags & 0x0f] << "," << tdi_str << "," << tdo_str << std::endl;
        } else {
            ss << time_str << ",," << JtagStateStr[frame.mFlags & 0x0f] << "," << tdi_str << "," << tdo_str << std::endl;
        }

        AnalyzerHelpers::AppendToFile((U8 *)ss.str().c_str(), (U32)ss.str().length(), f);
        ss.str(std::string());

        if (UpdateExportProgressAndCheckForCancel(i, num_frames) == true) {
            AnalyzerHelpers::EndFile(f);
            return;
        }
    }

    UpdateExportProgressAndCheckForCancel(num_frames, num_frames);
    AnalyzerHelpers::EndFile(f);
}

void SbwAnalyzerResults::GenerateFrameTabularText(U64 /*frame_index*/, DisplayBase /*display_base*/)
{
    ClearResultStrings();
    AddResultString("not supported");
}

void SbwAnalyzerResults::GeneratePacketTabularText(U64 /*packet_id*/, DisplayBase /*display_base*/)
{
    ClearResultStrings();
    AddResultString("not supported");
}

void SbwAnalyzerResults::GenerateTransactionTabularText(U64 /*transaction_id*/, DisplayBase /*display_base*/)
{
    ClearResultStrings();
    AddResultString("not supported");
}
