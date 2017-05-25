/*
 * VideoInput_TMDS442_NXP.c
 * for video (HDMI) input using the TI TMDS442 HDMI switch and the NXP 19971 HDMI receivers
 * Created: 7/20/2016 10:24:03 AM
 *  Author: Sensics
 */

#include "GlobalOptions.h"

#if defined(SVR_HAVE_NXP) && defined(SVR_HAVE_TMDS442)

#include "VideoInput.h"
#include "VideoInput_Protected.h"
#include "main.h"  // for the task booleans
#include "NXP/AVRHDMI.h"
#include "DeviceDrivers/TI-TMDS442.h"
#include "Console.h"
#include "SvrYield.h"
#include "SideBySide.h"
#include "FPGA.h"

void VideoInput_Init()
{
	/// Init TI TMDS442 HDMI input switch.
	TMDS442_Init();
	TMDS442_ForcePoll();

	/// Pull NXP chips out of reset.
	NXP_HDMI_Reset(1);
	NXP_HDMI_Reset(2);

	/// Init NXP HDMI receivers
	NXP_Init_HDMI();
}
static inline void VideoInput_dSight_dump_state(void)
{
	const uint8_t initialPlugSource = TMDS442_GetPlugSourceData();
	Write("VI State: ");
	switch (initialPlugSource)
	{
	case 0:
		Write("No plugs ");
		break;
	case 1:
		Write("Plug 1   ");
		break;
	case 2:
		Write("Plug 2   ");
		break;
	case 3:
		Write("Plug 1+2 ");
		break;
	default:
		Write("Plug Unk ");
	}
	if (VideoInput_Get_Status())
	{
		WriteLn("Locked");
	}
	else
	{
		WriteLn("Not locked");
	}
}
static inline bool needSxs(uint8_t plugSource)
{
	return (plugSource == TMDS442_PLUG_SOURCE_A) || (plugSource == TMDS442_PLUG_SOURCE_B);
}

static struct SxSWorkaround_s
{
	bool gotSwitchChange;
	bool wasVideoDetected;
} s_sxsWorkaround = {true, false};

static inline void sxsToggleWorkaround_doActualWorkaround(void)
{
	WriteLn("Performing side-by-side mode double toggle to enforce correct behavior.");
	SxS_Toggle();
	svr_yield_ms(10);
	SxS_Toggle();
	if (NXP_Get_HDMI_Status() == 1)
	{
		// Reset FPGA if we're in landscape.
		FPGA_start_reset();
		svr_yield_ms(20);
		FPGA_end_reset();
	}
}

static void sxsToggleWorkaround_signalSwitchChange(void) { s_sxsWorkaround.gotSwitchChange = true; }
/// For some reason, esp. with new FPGA code, dSight needs the side-by-side toggled twice when it gets a new display
/// signal that is single-port only.
static void sxsToggleWorkaround_task(void)
{
	bool wasVideoDetected = s_sxsWorkaround.wasVideoDetected;
	bool nowVideoDetected = VideoInput_Events.videoDetected;
	s_sxsWorkaround.wasVideoDetected = nowVideoDetected;

	/// This being true means that the main loop cleared the video detected event, so has presumably turned on displays,
	/// etc.
	bool videoDetectionHandled = (wasVideoDetected && !nowVideoDetected);

	if (videoDetectionHandled)
	{
		sxsToggleWorkaround_doActualWorkaround();
	}
	else if (nowVideoDetected && s_sxsWorkaround.gotSwitchChange)
	{
		sxsToggleWorkaround_doActualWorkaround();
		s_sxsWorkaround.gotSwitchChange = false;
	}

}
static void sxsToggleWorkaround_lateTask(void)
{
	if (!s_sxsWorkaround.wasVideoDetected && VideoInput_Events.videoDetected)
	{
	#if 0
		// No video at the start of this task, but now we have video.
		// Do the workaround.
		sxsToggleWorkaround_doActualWorkaround();
	#endif
	}
}

static inline void VideoInput_dSight_do_step(void)
{
	sxsToggleWorkaround_task();

	static bool gotTmdsChange = false;
	bool gotTmdsChangeLastTime = gotTmdsChange;
	gotTmdsChange = false;

	bool switchLostInput = false;
	VideoInput_dSight_dump_state();
	if (HDMISwitch_task)
	{
		// check status of HDMI switch
		// If it reports a change, reset NXP, report "no signal!", and exit the handler for now.
		// This will let plug/switch state changes properly propagate thru.
		const uint8_t initialPlugSource = TMDS442_GetPlugSourceData();
		if (TMDS442_Task())
		{
			/// If we got in here, the switch changed state.
			VideoInput_dSight_dump_state();
			sxsToggleWorkaround_signalSwitchChange();
			gotTmdsChange = true;
			const uint8_t newPlugSource = TMDS442_GetPlugSourceData();
			if (initialPlugSource != 0 && newPlugSource != 0)
			{
				// OK, so plugs changed without initial gain or complete loss - we will simulate a complete loss.
#if 0
				VideoInput_Protected_Report_No_Signal();
				NXP_HDMI_Reset(1);
				NXP_HDMI_Reset(2);
				#endif
				/// Init NXP HDMI receivers
				NXP_Init_HDMI();
				return;
			}
			else if (newPlugSource == 0)
			{
				// No longer have any input.
				// Will give NXP a chance to deal with this, but if it doesn't, we'll report it at the end.
				switchLostInput = true;
			}
		}
	}
	if (HDMI_task)
	{
		NXP_HDMI_Task();
	}

	sxsToggleWorkaround_lateTask();

	if (VideoInput_Events.videoDetected)
	{
		WriteLn("VideoInput: Video detected event, running task a few more times.");
		for (uint8_t i = 0; i < 10; ++i)
		{
			svr_yield_ms(10);
			NXP_HDMI_Task();
		}
	}

	if (switchLostInput && VideoInput_Get_Status())
	{
		// if we lost input, and videoinput still says we have video - report that we lost input here.
		WriteLn("VideoInput: TMDS detected full signal loss, but NXP did not. Reporting anyway.");
		VideoInput_Protected_Report_No_Signal();
	}
}
void VideoInput_Update_Resolution_Detection(void) { NXP_Update_Resolution_Detection(); }
void VideoInput_Task(void) {}
void VideoInput_Reset(uint8_t inputId) { NXP_HDMI_Reset(inputId); }
void VideoInput_Suspend(void) { NXP_Suspend(); }
void VideoInput_Resume(void) { NXP_Resume(); }
void VideoInput_Poll_Status(void)
{
	// All work is done in _Task
	VideoInput_dSight_do_step();
}
void VideoInput_Report_Status(void)
{
	NXP_Report_HDMI_status();
	VideoInput_dSight_dump_state();
}
#if 0
uint8_t VideoInput_Get_Detail_Status(void) { return Get_HDMI_Status(); }
#endif

#endif  // SVR_HAVE_NXP && SVR_HAVE_TMDS442
