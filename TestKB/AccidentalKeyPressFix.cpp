#include "stdafx.h"
#include "TlHelp32.h"
#include <map>
#include <iostream>
#include <thread>

LRESULT CALLBACK KeyboardHook(int, WPARAM, LPARAM);
void CALLBACK OsuMonitor();

typedef struct KeyState
{
	WPARAM lastIdentifier;
	DWORD lastDownTime;
} *PKeyState;

const DWORD MIN_INVERVAL = 100;

HHOOK gHook = NULL;
std::map<DWORD, KeyState> gKeyStateMap;
bool gIsOsuActive = false;

int main()
{
	std::thread monitor = std::thread(OsuMonitor);
	gHook = SetWindowsHookEx(WH_KEYBOARD_LL, KeyboardHook, NULL, NULL);

	MSG msg;
	while (GetMessage(&msg, NULL, NULL, NULL))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	UnhookWindowsHookEx(gHook);

	return 0;
}

LRESULT CALLBACK KeyboardHook(int nCode, WPARAM wParam, LPARAM lParam)
{
	bool skip = false;
	if (!gIsOsuActive && nCode == HC_ACTION)
	{
		PKBDLLHOOKSTRUCT kbEvent = (PKBDLLHOOKSTRUCT)lParam;
		PKeyState keyState = nullptr;
		auto iterator = gKeyStateMap.find(kbEvent->vkCode);

		if (iterator == gKeyStateMap.end())
		{
			keyState = &(gKeyStateMap[kbEvent->vkCode] = KeyState());
		}
		else
		{
			keyState = &(iterator->second);
			if (wParam == WM_KEYDOWN && keyState->lastIdentifier == WM_KEYUP && kbEvent->time - keyState->lastDownTime < MIN_INVERVAL)
			{
				skip = true;
				std::cout << "skipping for: " << (char)kbEvent->vkCode << ", at: " << kbEvent->time << std::endl;
			}
		}

		keyState->lastIdentifier = wParam;
		if (wParam == WM_KEYDOWN)
		{
			keyState->lastDownTime = kbEvent->time;
		}
	}

	return skip ? 1 : CallNextHookEx(gHook, nCode, wParam, lParam);
}

void CALLBACK OsuMonitor()
{
	const std::wstring osuExe = L"osu!.exe";
	PROCESSENTRY32 processInfo;
	processInfo.dwSize = sizeof(processInfo);
	HANDLE processesSnapshot = INVALID_HANDLE_VALUE;

	while (true)
	{
		bool foundOsuExe = false;
		processesSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, NULL);

		if (processesSnapshot != INVALID_HANDLE_VALUE)
		{
			if (Process32First(processesSnapshot, &processInfo))
			{
				do
				{
					foundOsuExe = (osuExe.compare(processInfo.szExeFile) == 0);
				} while (!foundOsuExe && Process32Next(processesSnapshot, &processInfo));
			}

			CloseHandle(processesSnapshot);
		}

		if (foundOsuExe != gIsOsuActive)
		{
			std::cout << "osu!.exe info: " << (foundOsuExe ? "opened" : "closed") << std::endl;
			gIsOsuActive = foundOsuExe;
		}

		Sleep(2500);
	}
}
