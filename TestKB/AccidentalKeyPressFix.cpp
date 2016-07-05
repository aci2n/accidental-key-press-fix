#include "stdafx.h"
#include <map>
#include <iostream>

LRESULT CALLBACK KeyboardHook(int, WPARAM, LPARAM);
struct KeyState
{
	WPARAM lastIdentifier;
	DWORD lastDownTime;
};

const DWORD MIN_INVERVAL = 100;

HHOOK gHook = NULL;
std::map<DWORD, KeyState> gKeyStateMap;

int main()
{
	MSG msg;
	gHook = SetWindowsHookEx(WH_KEYBOARD_LL, KeyboardHook, NULL, NULL);

	while (!GetMessage(&msg, NULL, NULL, NULL))
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

	if (nCode == HC_ACTION)
	{
		KBDLLHOOKSTRUCT *kbEvent = (KBDLLHOOKSTRUCT*)lParam;
		KeyState *keyState = nullptr;
		auto iterator = gKeyStateMap.find(kbEvent->vkCode);

		if (iterator == gKeyStateMap.end())
		{
			keyState = &(gKeyStateMap[kbEvent->vkCode] = KeyState());
		}
		else
		{
			keyState = &(iterator->second);
			skip = wParam == WM_KEYDOWN && keyState->lastIdentifier == WM_KEYUP && kbEvent->time - keyState->lastDownTime < MIN_INVERVAL;
		}

		keyState->lastIdentifier = wParam;
		if (wParam == WM_KEYDOWN)
		{
			keyState->lastDownTime = kbEvent->time;
		}
	}

	return skip ? 1 : CallNextHookEx(gHook, nCode, wParam, lParam);
}