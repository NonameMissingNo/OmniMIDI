﻿/*
OmniMIDI, a fork of BASSMIDI Driver

Thank you Kode54 for allowing me to fork your awesome driver.
*/

#pragma once

typedef unsigned __int64 QWORD;
typedef long NTSTATUS;

// KDMAPI version
#define CUR_MAJOR	2
#define CUR_MINOR	1
#define CUR_BUILD	0
#define CUR_REV		0

#define STRICT
#define VC_EXTRALEAN
#define WIN32_LEAN_AND_MEAN
#define __STDC_LIMIT_MACROS
#define _CRT_SECURE_CPP_OVERLOAD_STANDARD_NAMES 1

#define BASSASIODEF(f) (WINAPI *f)
#define BASSDEF(f) (WINAPI *f)
#define BASSENCDEF(f) (WINAPI *f)	
#define BASSMIDIDEF(f) (WINAPI *f)	
#define BASS_VSTDEF(f) (WINAPI *f)
#define Between(value, a, b) ((value) >= a && (value) <= b)

#define ERRORCODE		0
#define CAUSE			1
#define LONGMSG_MAXSIZE	65535

#define STATUS_SUCCESS 0
#define STATUS_TIMER_RESOLUTION_NOT_SET 0xC0000245

#include "stdafx.h"
#include <Psapi.h>
#include <atlbase.h>
#include <cstdint>
#include <comdef.h>
#include <fstream>
#include <iostream>
#include <codecvt>
#include <string>
#include <future>
#include <mmddk.h>
#include <process.h>
#include <shlobj.h>
#include <sstream>
#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <time.h>
#include <vector>
#include <algorithm>
#include <windows.h>
#include <Dbghelp.h>
#include <mmsystem.h>
#include <dsound.h>
#include <assert.h>
#include "Resource.h"
#include "OmniMIDI.h"
#include "RTSSOSD.h"

// BASS headers
#include <bass.h>
#include <bassmidi.h>
#include <bassenc.h>
#include <bassasio.h>
#include <bass_vst.h>

// Important
#include "LockSystem.h"
#include "Values.h"
#include "Debug.h"

// NTSTATUS
#define NT_SUCCESS(StatCode) ((NTSTATUS)(StatCode) == 0)
#define NTAPI __stdcall
// these functions have identical prototypes
typedef NTSTATUS(NTAPI* NDE)(BOOLEAN, INT64*);
typedef NTSTATUS(NTAPI* NQST)(QWORD*);
typedef NTSTATUS(NTAPI* DDP)(DWORD, HANDLE, UINT, LONG, LONG);

NDE NtDelayExecution = 0;
NQST NtQuerySystemTime = 0;
DDP DefDriverProcImp = 0;

// BASS (and plugins) handles
HMODULE bass = NULL, bassasio = NULL, bassenc = NULL, bassmidi = NULL, bass_vst = NULL;
HPLUGIN bassflac = NULL;

#define LOADLIBFUNCTION(l, f) *((void**)&f)=GetProcAddress(l,#f)

// Critical sections but handled by OmniMIDI functions because f**k Windows
DWORD DummyPlayBufData() { return 0; }
VOID DummyPrepareForBASSMIDI(DWORD LastRunningStatus, DWORD dwParam1) { return; }
MMRESULT DummyParseData(DWORD dwParam1) { return MIDIERR_NOTREADY; }
BOOL WINAPI DummyBMSE(HSTREAM handle, DWORD chan, DWORD event, DWORD param) { return FALSE; }

// Hyper switch
BOOL HyperMode = 0;
MMRESULT(*_PrsData)(DWORD dwParam1) = DummyParseData;
VOID(*_PforBASSMIDI)(DWORD LastRunningStatus, DWORD dwParam1) = DummyPrepareForBASSMIDI;
DWORD(*_PlayBufData)(void) = DummyPlayBufData;
DWORD(*_PlayBufDataChk)(void) = DummyPlayBufData;
BOOL(WINAPI*_BMSE)(HSTREAM handle, DWORD chan, DWORD event, DWORD param) = DummyBMSE;
// What does it do? It gets rid of the useless functions,
// and passes the events without checking for anything

// F**k Sleep() tbh
void NTSleep(__int64 usec) {
	__int64 neg = (usec * -1);
	NtDelayExecution(FALSE, &neg);
}

// Predefined sleep values, useful for redundancy
#define _FWAIT NTSleep(1)								// Fast wait
#define _WAIT NTSleep(100)								// Normal wait
#define _SWAIT NTSleep(5000)							// Slow wait
#define _CFRWAIT NTSleep(16667)							// Cap framerate wait

// Variables
#include "BASSErrors.h"

// OmniMIDI vital parts
#include "SoundFontLoader.h"
#include "BufferSystem.h"
#include "Settings.h"
#include "BlacklistSystem.h"
#include "DriverInit.h"
#include "KDMAPI.h"

BOOL APIENTRY DllMain(HMODULE hModule, DWORD CallReason, LPVOID lpReserved)
{
	switch (CallReason) {
	case DLL_PROCESS_ATTACH:
	{
		if (BannedProcesses()) {
			OutputDebugStringA("Process is banned! Bye!");
			return FALSE;
		}

		hinst = hModule;
		DisableThreadLibraryCalls(hinst);
		BASS_MIDI_StreamEvent = DummyBMSE;

		if (!NtDelayExecution || !NtQuerySystemTime) {
			NtDelayExecution = (NDE)GetProcAddress(GetModuleHandleW(L"ntdll"), "NtDelayExecution");
			NtQuerySystemTime = (NQST)GetProcAddress(GetModuleHandleW(L"ntdll"), "NtQuerySystemTime");

			if (!NtDelayExecution || !NtQuerySystemTime) {
				OutputDebugStringA("Failed to parse NT functions from NTDLL!\nPress OK to stop the loading process of OmniMIDI.");
				return FALSE;
			}

			if (!NT_SUCCESS(NtQuerySystemTime(&TickStart))) {
				OutputDebugStringA("Failed to parse starting tick through NtQuerySystemTime!\nPress OK to stop the loading process of OmniMIDI.");
				return FALSE;
			}

			// Loaded!
			return TRUE;
		}

		break;
	}
	case DLL_PROCESS_DETACH:
	{
		hinst = NULL;
		break;
	}
	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH: 
		break;
	}

	return TRUE;
}

LONG WINAPI DriverProc(DWORD dwDriverIdentifier, HANDLE hdrvr, UINT uMsg, LONG lParam1, LONG lParam2)
{
	switch (uMsg) {
	case DRV_LOAD:
	case DRV_FREE:
		return DRVCNF_OK;

	case DRV_OPEN:
	case DRV_CLOSE:
		return DRVCNF_OK;

	case DRV_QUERYCONFIGURE:
		return DRVCNF_OK;

	case DRV_CONFIGURE:
		TCHAR configuratorapp[MAX_PATH];
		if (SUCCEEDED(SHGetFolderPath(NULL, CSIDL_SYSTEMX86, NULL, 0, configuratorapp)))
		{
			PathAppend(configuratorapp, _T("\\OmniMIDI\\OmniMIDIConfigurator.exe"));
			ShellExecute(NULL, L"open", configuratorapp, L"/AST", NULL, SW_SHOWNORMAL);
			return DRVCNF_OK;
		}
		return DRVCNF_CANCEL;

	case DRV_ENABLE:
	case DRV_DISABLE:
		return DRVCNF_OK;

	case DRV_INSTALL:
		return DRVCNF_OK;

	case DRV_REMOVE:
		return DRVCNF_OK;
	}

	if (!winmm) {
		winmm = GetModuleHandleA("winmm");
		if (!winmm) {
			winmm = LoadLibraryA("winmm");
			if (!winmm) {
				MessageBoxA(NULL, "Failed to load WinMM!\nPress OK to stop the loading process of OmniMIDI.", "OmniMIDI - ERROR", MB_ICONERROR | MB_SYSTEMMODAL);
				return FALSE;
			}
		}

		DefDriverProcImp = (DDP)GetProcAddress(winmm, "DefDriverProc");
		if (!DefDriverProcImp) {
			MessageBoxA(NULL, "Failed to parse DefDriverProc function from WinMM!\nPress OK to stop the loading process of OmniMIDI.", "OmniMIDI - ERROR", MB_ICONERROR | MB_SYSTEMMODAL);
			return FALSE;
		}
	}

	return DefDriverProcImp(dwDriverIdentifier, (HDRVR)hdrvr, uMsg, lParam1, lParam2);
}

DWORD GiveOmniMIDICaps(PVOID capsPtr, DWORD capsSize) {
	// Initialize values
	static BOOL LoadedOnce = FALSE;

	BOOL DefSynthType = FALSE;
	DWORD StreamCapable = NULL;
	WORD Technology = NULL;
	WORD MID = 0x0000;
	WORD PID = 0x0000;

	try {
		PrintMessageToDebugLog("MODM_GETDEVCAPS", "The MIDI app sent a MODM_GETDEVCAPS request to the driver.");

		if (!LoadedOnce) {
			OpenRegistryKey(Configuration, L"Software\\OmniMIDI\\Configuration", FALSE);
			RegQueryValueEx(Configuration.Address, L"DebugMode", NULL, &dwType, (LPBYTE)&ManagedSettings.DebugMode, &dwSize);
			RegQueryValueEx(Configuration.Address, L"DisableChime", NULL, &dwType, (LPBYTE)&DisableChime, &dwSize);
			RegQueryValueEx(Configuration.Address, L"DisableCookedPlayer", NULL, &dwType, (LPBYTE)&ManagedSettings.DisableCookedPlayer, &dwSize);
			RegQueryValueEx(Configuration.Address, L"PID", NULL, &dwType, (LPBYTE)&PID, &dwSize);
			RegQueryValueEx(Configuration.Address, L"SynthName", NULL, &SNType, (LPBYTE)&SynthNameW, &SNSize);
			RegQueryValueEx(Configuration.Address, L"SynthType", NULL, &dwType, (LPBYTE)&SynthType, &dwSize);
			RegQueryValueEx(Configuration.Address, L"VID", NULL, &dwType, (LPBYTE)&MID, &dwSize);

			// If the debug mode is enabled, and the process isn't banned, create the debug log
			if (ManagedSettings.DebugMode && BlackListSystem())
				CreateConsole();

			// If the synth type ID is bigger than the size of the synth types array,
			// set it automatically to MOD_MIDIPORT
			DefSynthType = (SynthType < 0 || SynthType >= ((sizeof(SynthNamesTypes) / sizeof(*SynthNamesTypes))));
			if (SynthType < 0 || SynthType >= ((sizeof(SynthNamesTypes) / sizeof(*SynthNamesTypes))))
				Technology = MOD_MIDIPORT;
			// Else, load the requested value
			else Technology = SynthNamesTypes[SynthType];
			PrintStreamValueToDebugLog("MODM_GETDEVCAPS", "Technology", Technology);

			// If the synthname length is less than 1, or if it's just a space, use the default name
			PrintMessageToDebugLog("MODM_GETDEVCAPS", "Checking if SynthNameW is valid...");
			if (!wcslen(SynthNameW) || (wcslen(SynthNameW) && iswspace(SynthNameW[0]))) {
				RtlSecureZeroMemory(SynthNameW, sizeof(SynthNameW));
				wcsncpy(SynthNameW, L"OmniMIDI\0", MAXPNAMELEN);
			}

			StreamCapable = (ManagedSettings.DisableCookedPlayer || CPBlacklisted) ? 0 : MIDICAPS_STREAM;
			if (!StreamCapable)
				PrintMessageToDebugLog("MODM_GETDEVCAPS", "Either the app is blacklisted, or the user requested to disable CookedPlayer globally.");

			LoadedOnce = TRUE;
		}

		PrintMessageToDebugLog("MODM_GETDEVCAPS", "Sharing MIDI device caps with application...");

		// Prepare the caps item
		switch (capsSize) {
		case (sizeof(MIDIOUTCAPSA)):
		{
			if (capsPtr == NULL || capsSize != sizeof(MIDIOUTCAPSA))
				return MMSYSERR_INVALPARAM;

			MIDIOUTCAPSA MIDICaps;

			wcstombs(MIDICaps.szPname, SynthNameW, MAXPNAMELEN);
			MIDICaps.dwSupport = StreamCapable | MIDICAPS_VOLUME;
			MIDICaps.wChannelMask = 0xFFFF;
			MIDICaps.wMid = MID;
			MIDICaps.wPid = PID;
			MIDICaps.wTechnology = DefSynthType ? MOD_MIDIPORT : SynthNamesTypes[SynthType];
			MIDICaps.wNotes = 0;
			MIDICaps.wVoices = 0;
			MIDICaps.vDriverVersion = MAKEWORD(6, 0);

			memcpy((LPMIDIOUTCAPSA)capsPtr, &MIDICaps, min(capsSize, sizeof(MIDICaps)));
			PrintMessageToDebugLog("MODM_GETDEVCAPS (ASCII, Type 1)", "Done sharing MIDI device caps.");

			break;
		}
		case (sizeof(MIDIOUTCAPSW)):
		{
			if (capsPtr == NULL || capsSize != sizeof(MIDIOUTCAPSW))
				return MMSYSERR_INVALPARAM;

			MIDIOUTCAPSW MIDICaps;

			wcsncpy(MIDICaps.szPname, SynthNameW, MAXPNAMELEN);
			MIDICaps.dwSupport = StreamCapable | MIDICAPS_VOLUME;
			MIDICaps.wChannelMask = 0xFFFF;
			MIDICaps.wMid = MID;
			MIDICaps.wPid = PID;
			MIDICaps.wTechnology = DefSynthType ? MOD_MIDIPORT : SynthNamesTypes[SynthType];
			MIDICaps.wNotes = 0;
			MIDICaps.wVoices = 0;
			MIDICaps.vDriverVersion = MAKEWORD(6, 0);

			memcpy((LPMIDIOUTCAPSW)capsPtr, &MIDICaps, min(capsSize, sizeof(MIDICaps)));
			PrintMessageToDebugLog("MODM_GETDEVCAPS (Unicode, Type 1)", "Done sharing MIDI device caps.");

			break;
		}
		case (sizeof(MIDIOUTCAPS2A)):
		{	
			if (capsPtr == NULL || capsSize != sizeof(MIDIOUTCAPS2A))
				return MMSYSERR_INVALPARAM;

			MIDIOUTCAPS2A MIDICaps;

			wcstombs(MIDICaps.szPname, SynthNameW, MAXPNAMELEN);
			MIDICaps.ManufacturerGuid = OMCLSID;
			MIDICaps.NameGuid = OMCLSID;
			MIDICaps.ProductGuid = OMCLSID;
			MIDICaps.dwSupport = StreamCapable | MIDICAPS_VOLUME;
			MIDICaps.wChannelMask = 0xFFFF;
			MIDICaps.wMid = MID;
			MIDICaps.wPid = PID;
			MIDICaps.wTechnology = DefSynthType ? MOD_MIDIPORT : SynthNamesTypes[SynthType];
			MIDICaps.wNotes = 0;
			MIDICaps.wVoices = 0;
			MIDICaps.vDriverVersion = MAKEWORD(6, 0);

			memcpy((LPMIDIOUTCAPS2A)capsPtr, &MIDICaps, min(capsSize, sizeof(MIDICaps)));
			PrintMessageToDebugLog("MODM_GETDEVCAPS (ASCII, Type 2)", "Done sharing MIDI device caps.");

			break;
		}
		case (sizeof(MIDIOUTCAPS2W)):
		{	
			if (capsPtr == NULL || capsSize != sizeof(MIDIOUTCAPS2W))
				return MMSYSERR_INVALPARAM;

			MIDIOUTCAPS2W MIDICaps;

			wcsncpy(MIDICaps.szPname, SynthNameW, MAXPNAMELEN);
			MIDICaps.ManufacturerGuid = OMCLSID;
			MIDICaps.NameGuid = OMCLSID;
			MIDICaps.ProductGuid = OMCLSID;
			MIDICaps.dwSupport = StreamCapable | MIDICAPS_VOLUME;
			MIDICaps.wChannelMask = 0xFFFF;
			MIDICaps.wMid = MID;
			MIDICaps.wPid = PID;
			MIDICaps.wTechnology = DefSynthType ? MOD_MIDIPORT : SynthNamesTypes[SynthType];
			MIDICaps.wNotes = 0;
			MIDICaps.wVoices = 0;
			MIDICaps.vDriverVersion = MAKEWORD(6, 0);

			memcpy((LPMIDIOUTCAPS2W)capsPtr, &MIDICaps, min(capsSize, sizeof(MIDICaps)));
			PrintMessageToDebugLog("MODM_GETDEVCAPS (Unicode, Type 2)", "Done sharing MIDI device caps.");

			break;
		}
		}

		return MMSYSERR_NOERROR;
	}
	catch (...) {
		return MMSYSERR_NOTENABLED;
	}
}

MMRESULT DequeueMIDIHDRs(DWORD_PTR dwUser) 
{
	if (!dwUser)
		return DebugResult("DequeueMIDIHDRs", MMSYSERR_INVALPARAM, "dwUser is not valid.");

	for (LPMIDIHDR hdr = ((CookedPlayer*)dwUser)->MIDIHeaderQueue; hdr; hdr = hdr->lpNext)
	{
		LockForWriting(&((CookedPlayer*)dwUser)->Lock);
		PrintMessageToDebugLog("MODM_RESET", "Marking buffer as done and not in queue anymore...");
		hdr->dwFlags &= ~MHDR_INQUEUE;
		hdr->dwFlags |= MHDR_DONE;
		UnlockForWriting(&((CookedPlayer*)dwUser)->Lock);

		DoCallback(MOM_DONE, (DWORD_PTR)hdr, 0);
	}

	return MMSYSERR_NOERROR;
}

MMRESULT modMessage(UINT uDeviceID, UINT uMsg, DWORD_PTR dwUser, DWORD_PTR dwParam1, DWORD_PTR dwParam2)
{
	static BOOL PreventInit = FALSE;
	MMRESULT RetVal = MMSYSERR_NOERROR;
	
	/*
	char* Msg = (char*)malloc(sizeof(char) * NTFS_MAX_PATH);
	sprintf(Msg, "Received modMessage(%u, %u, %X, %X, %X)", uDeviceID, uMsg, dwUser, dwParam1, dwParam2);
	PrintMessageToDebugLog("MOD_MESSAGE", Msg);
	free(Msg);
	*/

	switch (uMsg) {
	case MODM_DATA:
		// Parse the data lol
		return _PrsData(dwParam1);
	case MODM_LONGDATA: {
		// Pass it to a KDMAPI function
		RetVal = SendDirectLongData((MIDIHDR*)dwParam1);

		// Tell the app that the buffer has been played
		DoCallback(MOM_DONE, dwParam1, 0);
		// if (CustomCallback) CustomCallback((HMIDIOUT)OMMOD.hMidi, MM_MOM_DONE, WMMCI, dwParam1, 0);
		return RetVal;
	}
	case MODM_STRMDATA: {
		if (!bass_initialized || !dwUser)
			return DebugResult("MODM_STRMDATA", MIDIERR_NOTREADY, "You can't call midiStreamData with a normal MIDI stream, or the driver isn't ready.");

		if ((DWORD)dwParam2 < offsetof(MIDIHDR, dwOffset) ||
			!((MIDIHDR*)dwParam1) || !((MIDIHDR*)dwParam1)->lpData ||
			((MIDIHDR*)dwParam1)->dwBufferLength < ((MIDIHDR*)dwParam1)->dwBytesRecorded ||
			((MIDIHDR*)dwParam1)->dwBytesRecorded % 4)
		{
			return DebugResult("MODM_STRMDATA", MMSYSERR_INVALPARAM, "The buffer doesn't exist, hasn't been allocated or is not valid.");
		}

		if (!(((MIDIHDR*)dwParam1)->dwFlags & MHDR_PREPARED))
			return DebugResult("MODM_STRMDATA", MIDIERR_UNPREPARED, "The buffer is not prepared.");

		if (!(((MIDIHDR*)dwParam1)->dwFlags & MHDR_DONE)) {
			if (((MIDIHDR*)dwParam1)->dwFlags & MHDR_INQUEUE)
				return DebugResult("MODM_STRMDATA", MIDIERR_STILLPLAYING, "The buffer is still being played.");
		}

		PrintMessageToDebugLog("MODM_STRMDATA", "Locking for writing...");
		LockForWriting(&((CookedPlayer*)dwUser)->Lock);

		PrintMessageToDebugLog("MODM_STRMDATA", "Copying pointer of buffer...");

		PrintMIDIHDRToDebugLog("MODM_STRMDATA", (MIDIHDR*)dwParam1);

		((MIDIHDR*)dwParam1)->dwFlags &= ~MHDR_DONE;
		((MIDIHDR*)dwParam1)->dwFlags |= MHDR_INQUEUE;
		((MIDIHDR*)dwParam1)->lpNext = 0;
		((MIDIHDR*)dwParam1)->dwOffset = 0;
		if (((CookedPlayer*)dwUser)->MIDIHeaderQueue)
		{
			PrintMessageToDebugLog("MODM_STRMDATA", "Another buffer is already present. Adding it to queue...");
			LPMIDIHDR phdr = ((CookedPlayer*)dwUser)->MIDIHeaderQueue;

			if (phdr == (MIDIHDR*)dwParam1) {
				PrintMessageToDebugLog("MODM_STRMDATA", "Unlocking...");
				UnlockForWriting(&((CookedPlayer*)dwUser)->Lock);
				return MIDIERR_STILLPLAYING;
			}
			while (phdr->lpNext)
			{
				phdr = phdr->lpNext;
				if (phdr == (MIDIHDR*)dwParam1)
				{
					PrintMessageToDebugLog("MODM_STRMDATA", "Unlocking...");
					UnlockForWriting(&((CookedPlayer*)dwUser)->Lock);
					return MIDIERR_STILLPLAYING;
				}
			}
			phdr->lpNext = (MIDIHDR*)dwParam1;
		}
		else ((CookedPlayer*)dwUser)->MIDIHeaderQueue = (MIDIHDR*)dwParam1;
		PrintMessageToDebugLog("MODM_STRMDATA", "Copied.");

		PrintMessageToDebugLog("MODM_STRMDATA", "Unlocking...");
		UnlockForWriting(&((CookedPlayer*)dwUser)->Lock);

		PrintMessageToDebugLog("MODM_STRMDATA", "All done!");
		return MMSYSERR_NOERROR;
	}
	case MODM_PROPERTIES: {
		if (!bass_initialized || !dwUser)
			return DebugResult("MODM_PROPERTIES", MIDIERR_NOTREADY, "You can't call midiStreamProperties with a normal MIDI stream, or the driver isn't ready.");

		if (!((DWORD)dwParam2 & (MIDIPROP_GET | MIDIPROP_SET)))
			return DebugResult("MODM_PROPERTIES", MMSYSERR_INVALPARAM, "The MIDI application is confused, and didn't specify if it wanted to get the properties or set them.");

		if ((DWORD)dwParam2 & MIDIPROP_TEMPO) {
			MIDIPROPTEMPO* MPropTempo = (MIDIPROPTEMPO*)dwParam1;

			if (sizeof(MIDIPROPTEMPO) != MPropTempo->cbStruct) {
				return DebugResult("MODM_PROPERTIES", MMSYSERR_INVALPARAM, "Invalid pointer to MIDIPROPTEMPO struct.");
			}
			else if ((DWORD)dwParam2 & MIDIPROP_SET) {
				PrintMessageToDebugLog("MODM_PROPERTIES", "CookedPlayer's tempo set to received value.");
				((CookedPlayer*)dwUser)->Tempo = MPropTempo->dwTempo;
				PrintStreamValueToDebugLog("MODM_PROPERTIES", "Received Tempo", ((CookedPlayer*)dwUser)->Tempo);
				((CookedPlayer*)dwUser)->TempoMulti = ((((CookedPlayer*)dwUser)->Tempo * 10) / ((CookedPlayer*)dwUser)->TimeDiv);
				PrintStreamValueToDebugLog("MODM_PROPERTIES", "New TempoMulti", ((CookedPlayer*)dwUser)->TempoMulti);
			}
			else if ((DWORD)dwParam2 & MIDIPROP_GET) {
				PrintMessageToDebugLog("MODM_PROPERTIES", "CookedPlayer's tempo sent to MIDI application.");
				MPropTempo->dwTempo = ((CookedPlayer*)dwUser)->Tempo;
			}
		}
		else if ((DWORD)dwParam2 & MIDIPROP_TIMEDIV) {
			MIDIPROPTIMEDIV* MPropTimeDiv = (MIDIPROPTIMEDIV*)dwParam1;

			if (sizeof(MIDIPROPTIMEDIV) != MPropTimeDiv->cbStruct) {
				return DebugResult("MODM_PROPERTIES", MMSYSERR_INVALPARAM, "Invalid pointer to MIDIPROPTIMEDIV struct.");
			}
			else if ((DWORD)dwParam2 & MIDIPROP_SET) {
				PrintMessageToDebugLog("MODM_PROPERTIES", "CookedPlayer's time division set to received value.");
				((CookedPlayer*)dwUser)->TimeDiv = MPropTimeDiv->dwTimeDiv;
				PrintStreamValueToDebugLog("MODM_PROPERTIES", "Received TimeDiv", ((CookedPlayer*)dwUser)->TimeDiv);
				((CookedPlayer*)dwUser)->TempoMulti = ((((CookedPlayer*)dwUser)->Tempo * 10) / ((CookedPlayer*)dwUser)->TimeDiv);
				PrintStreamValueToDebugLog("MODM_PROPERTIES", "New TempoMulti", ((CookedPlayer*)dwUser)->TempoMulti);
			}
			else if ((DWORD)dwParam2 & MIDIPROP_GET) {
				PrintMessageToDebugLog("MODM_PROPERTIES", "CookedPlayer's time division sent to MIDI application.");
				MPropTimeDiv->dwTimeDiv = ((CookedPlayer*)dwUser)->TimeDiv;
			}
		}
		else return DebugResult("MODM_PROPERTIES", MMSYSERR_INVALPARAM, "Invalid properties.");

		return MMSYSERR_NOERROR;
	}
	case MODM_GETPOS: {
		if (!bass_initialized || !dwUser)
			return DebugResult("MODM_GETPOS", MIDIERR_NOTREADY, "You can't call midiStreamPosition with a normal MIDI stream, or the driver isn't ready.");

		if (!dwParam1 || !dwParam2)
			return DebugResult("MODM_GETPOS", MMSYSERR_INVALPARAM, "Invalid parameters.");

		PrintMessageToDebugLog("MODM_GETPOS", "The app wants to know the current position of the stream.");

		RESET:
		switch (((MMTIME*)dwParam1)->wType) {
		case TIME_BYTES:
			PrintMessageToDebugLog("TIME_BYTES", "The app wanted it in bytes.");
			((MMTIME*)dwParam1)->u.cb = ((CookedPlayer*)dwUser)->ByteAccumulator;
			break;
		case TIME_MS:
			PrintMessageToDebugLog("TIME_MS", "The app wanted it in milliseconds.");
			((MMTIME*)dwParam1)->u.ms = ((CookedPlayer*)dwUser)->TimeAccumulator / 10000;
			break;
		case TIME_TICKS:
			PrintMessageToDebugLog("TIME_TICKS", "The app wanted it in ticks.");
			((MMTIME*)dwParam1)->u.ticks = ((CookedPlayer*)dwUser)->TickAccumulator;
			break;
		default:
			PrintMessageToDebugLog("TIME_UNK", "Unrecognized wType. Parsing in the default format of ticks.");
			((MMTIME*)dwParam1)->wType = TIME_TICKS;
			goto RESET;
			break;
		}

		PrintMessageToDebugLog("MODM_GETPOS", "The app now knows the position.");
		return MMSYSERR_NOERROR;
	}
	case MODM_RESTART: {
		if (!bass_initialized || !dwUser)
			return DebugResult("MODM_RESTART", MMSYSERR_NOTENABLED, "You can't call midiStreamRestart with a normal MIDI stream, or the driver isn't ready.");

		if (((CookedPlayer*)dwUser)->Paused) {
			((CookedPlayer*)dwUser)->Paused = FALSE;
			PrintMessageToDebugLog("MODM_RESTART", "CookedPlayer is now playing.");
		}

		return MMSYSERR_NOERROR;
	}
	case MODM_PAUSE: {
		if (!bass_initialized || !dwUser)
			return DebugResult("MODM_PAUSE", MMSYSERR_NOTENABLED, "You can't call midiStreamPause with a normal MIDI stream, or the driver isn't ready.");

		if (!((CookedPlayer*)dwUser)->Paused) {
			((CookedPlayer*)dwUser)->Paused = TRUE;
			ResetSynth(FALSE);
			PrintMessageToDebugLog("MODM_PAUSE", "CookedPlayer is now paused.");
		}

		return MMSYSERR_NOERROR;
	}
	case MODM_STOP: {
		if (!bass_initialized || !dwUser)
			return DebugResult("MODM_STOP", MIDIERR_NOTREADY, "You can't call midiStreamStop with a normal MIDI stream, or the driver isn't ready.");

		PrintMessageToDebugLog("MODM_STOP", "The app requested OmniMIDI to stop CookedPlayer.");
		((CookedPlayer*)dwUser)->Paused = TRUE;

		ResetSynth(FALSE);
		RetVal = DequeueMIDIHDRs(dwUser);

		if (!RetVal) PrintMessageToDebugLog("MODM_STOP", "CookedPlayer is now stopped.");
		return RetVal;
	}
	case MODM_RESET:
		// Stop all the current active voices
		ResetSynth(FALSE);

		PrintMessageToDebugLog("MODM_RESET", (dwUser ? "The app requested OmniMIDI to reset CookedPlayer." : "The app sent a reset command."));
		return (dwUser ? DequeueMIDIHDRs(dwUser) : MMSYSERR_NOERROR);
	case MODM_PREPARE:
		// Pass it to a KDMAPI function
		return PrepareLongData((MIDIHDR*)dwParam1);
	case MODM_UNPREPARE:
		// Pass it to a KDMAPI function
		return UnprepareLongData((MIDIHDR*)dwParam1);
	case MODM_GETNUMDEVS:
		// Return "1" if the process isn't blacklisted, otherwise the driver doesn't exist OwO
		return BlackListSystem();
	case MODM_GETDEVCAPS:
		// Return OM's caps to the app
		return GiveOmniMIDICaps((PVOID)dwParam1, (DWORD)dwParam2);
	case MODM_GETVOLUME: {
		// Tell the app the current output volume of the driver
		PrintMessageToDebugLog("MODM_GETVOLUME", "The app wants to know the current output volume of the driver.");
		*(LONG*)dwParam1 = (LONG)(SynthVolume * 0xFFFF);
		PrintMessageToDebugLog("MODM_GETVOLUME", "The app knows the volume now.");
		return MMSYSERR_NOERROR;
	}
	case MODM_SETVOLUME: {
		// The app isn't allowed to set the volume, everything's fine anyway
		PrintMessageToDebugLog("MODM_SETVOLUME", "Dummy, the app has no control over the driver's audio output.");
		return MMSYSERR_NOERROR;
	}
	case MODM_OPEN: {
		if (PreventInit) {
			PrintMessageToDebugLog("MODM_OPEN", "The app is dumb and requested to open the stream again during the initialization process...");
			return MIDIERR_NOTREADY;
		}

		PrintMessageToDebugLog("MODM_OPEN", "The app requested the driver to initialize its audio stream.");
		if (!AlreadyInitializedViaKDMAPI && !bass_initialized) {
			// Prevent the app from calling MODM_OPEN again...
			PreventInit = TRUE;

			// Parse callback and instance
			// AddVectoredExceptionHandler(1, OmniMIDICrashHandler);
			PrintMessageToDebugLog("MODM_OPEN", "Preparing callback data (If present)...");
			LPMIDIOPENDESC OMMPD = ((MIDIOPENDESC*)dwParam1);
			PrintMIDIOPENDESCToDebugLog("MODM_OPEN", OMMPD, dwUser, (DWORD)dwParam2);

			InitializeCallbackFeatures(
				OMMPD->hMidi,
				OMMPD->dwCallback,
				OMMPD->dwInstance,
				dwUser,
				(DWORD)dwParam2);

			// Enable handler if required
			EnableBuiltInHandler("MODM_OPEN");

			// Open the driver
			PrintMessageToDebugLog("MODM_OPEN", "Initializing driver...");
			DoStartClient();
			ResetSynth(TRUE);

			// Tell the app that the driver is ready
			PrintMessageToDebugLog("MODM_OPEN", "Sending callback data to app. if needed...");
			DoCallback(MOM_OPEN, 0, 0);

			PrintMessageToDebugLog("MODM_OPEN", "Everything is fine.");
			PreventInit = FALSE;
		}
		else {
			PreventInit = FALSE;
			PrintMessageToDebugLog("MODM_OPEN", "The driver has already been initialized.");
		}

		return MMSYSERR_NOERROR;
	}
	case MODM_CLOSE: {
		if (PreventInit) DebugResult("MODM_CLOSE", MMSYSERR_ALLOCATED, "The driver has already been initialized. Cannot initialize it twice!");

		if (!AlreadyInitializedViaKDMAPI) {
			// Prevent the app from calling MODM_CLOSE again...
			PreventInit = TRUE;

			PrintMessageToDebugLog("MODM_CLOSE", "The app requested the driver to terminate its audio stream.");
			ResetSynth(TRUE);

			if (bass_initialized) {
				PrintMessageToDebugLog("MODM_CLOSE", "Terminating driver...");
				KillOldCookedPlayer(dwUser);
				DoStopClient();
				DisableBuiltInHandler("MODM_CLOSE");
			}

			DoCallback(MOM_CLOSE, 0, 0);

			PrintMessageToDebugLog("MODM_CLOSE", "Everything is fine.");
		}
		else PrintMessageToDebugLog("MODM_CLOSE", "The driver is already in use via KDMAPI. Cannot terminate it!");

		PreventInit = FALSE;
		return DebugResult("MODM_CLOSE", MMSYSERR_NOERROR, "The driver has been stopped.");
	}
	case MODM_CACHEPATCHES:
	case MODM_CACHEDRUMPATCHES:
	case DRV_QUERYDEVICEINTERFACESIZE:
	case DRV_QUERYDEVICEINTERFACE:
		return MMSYSERR_NOERROR;
	default: {
		// Unrecognized uMsg
		char* Msg = (char*)malloc(sizeof(char) * NTFS_MAX_PATH);
		sprintf(Msg, 
			"The application sent an unknown message! ID: 0x%08x - dwUser: 0x%08x - dwParam1: 0x%08x - dwParam2: 0x%08x", 
			uMsg, dwUser, dwParam1, dwParam2);
		RetVal = DebugResult("modMessage", MMSYSERR_ERROR, Msg);
		free(Msg);
		return RetVal;
	}
	}
}