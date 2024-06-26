#include "windel.h"

#include <commctrl.h>

#include <stdio.h>
#include <tchar.h>
#include <stdlib.h>
#include <assert.h>

#include "resource.h"

struct WinDelState
{
	HINSTANCE hInstance;
	HANDLE    hTDSync;
	HWND      hTaskDialog;
	HANDLE    hOldDesk;
	HANDLE    hNewDesk;
	short     nCurrent;
	short     nTotal;
	BOOL      bEnd;
};

static void ThisIsJustJoke(HINSTANCE hThisInstance)
{
	TCHAR szMessage[1024];

	LoadString(hThisInstance, IDS_JUST_JOKE, szMessage, 1024);
	MessageBox(NULL, szMessage, L" ", MB_ICONASTERISK);
}


static void init_desktops(struct WinDelState *state);
static void switch_desk(struct WinDelState *state);
static void switch_back(struct WinDelState *state);

static
DWORD windel_window(struct WinDelState *state);

static
DWORD delete_windows(struct WinDelState *state);

int DeleteWindows(HINSTANCE hThisInstance)
{
	HANDLE hWinDelThreads[2];
	DWORD code = 0;
	struct WinDelState *state = NULL;

	state = LocalAlloc(LPTR, sizeof(struct WinDelState));
	if (state == NULL) {
		MessageBoxW(NULL, L"An error has been occured "
			L"during Windows removal initialization process.",
			L"Error", MB_ICONHAND);	
		return 1;
	}

	state->hInstance   = hThisInstance;
	state->hTDSync       = CreateSemaphoreW(NULL, 1, 1, L"WinDelSem");
	state->hTaskDialog = NULL;
	state->nCurrent    = 0;
	state->nTotal      = 1000;
	
	hWinDelThreads[0] = CreateThread(
		NULL,
		0,
		windel_window,
		state,
		0,
		NULL);

	hWinDelThreads[1] = CreateThread(
		NULL,
		0,
		delete_windows,
		state,
		0,
		NULL);

	WaitForMultipleObjects(2, hWinDelThreads, TRUE, INFINITE);

	ThisIsJustJoke(hThisInstance);

	if (!GetExitCodeThread(hWinDelThreads[1], &code))
		code = -1;

cleanup:
	CloseHandle(hWinDelThreads[0]);
	CloseHandle(hWinDelThreads[1]);
	CloseHandle(state->hTDSync);
	LocalFree(state);

	return code;
}


static
HRESULT WINAPI
windel_callback(
	HWND hWnd, 
	UINT uMsg, 
	WPARAM wParam, 
	LPARAM lParam, 
	LONG_PTR lpRefData
)
{
	struct WinDelState *state = (struct WinDelState *)lpRefData;

	switch (uMsg) {
	case TDN_CREATED:
		state->hTaskDialog = hWnd;
		SendMessage(hWnd, TDM_ENABLE_BUTTON, IDOK, 0);
		SendMessage(state->hTaskDialog, TDM_SET_PROGRESS_BAR_RANGE, 
			0, MAKELPARAM(0, state->nTotal));
		ReleaseSemaphore(state->hTDSync, 1, NULL);
		break;
	case TDN_TIMER:
		SendMessage(state->hTaskDialog, TDM_SET_PROGRESS_BAR_POS,
			state->nCurrent, 0);
		if (!state->bEnd && state->nCurrent >= state->nTotal) {
			SendMessage(hWnd, 
				TDM_SET_PROGRESS_BAR_POS,
				state->nTotal, 0);
			SendMessage(hWnd, TDM_SET_ELEMENT_TEXT, 
					TDE_MAIN_INSTRUCTION,
					MAKEINTRESOURCE(IDS_END_MSG));
			state->bEnd = TRUE;
			return S_FALSE;
		}
		if (state->bEnd && wParam >= 5000) {
			EndDialog(hWnd, 0);
		}
		return S_OK;
	}
	return S_FALSE;
}

static
DWORD windel_window(struct WinDelState *state)
{
	TASKDIALOGCONFIG tdcfg = {0};

	tdcfg.cbSize = sizeof(tdcfg);
	tdcfg.hInstance = state->hInstance;
	tdcfg.dwFlags = TDF_SHOW_PROGRESS_BAR | TDF_CALLBACK_TIMER;
	tdcfg.pszWindowTitle = MAKEINTRESOURCE(IDS_DELETING_TITLE);
	tdcfg.pszMainInstruction = MAKEINTRESOURCE(IDS_DELETING_MSG);
	tdcfg.pfCallback = windel_callback;
	tdcfg.lpCallbackData = state;
	
	init_desktops(state);
	switch_desk(state);

	TaskDialogIndirect(&tdcfg, NULL, NULL, NULL);

	Sleep(2000);
	switch_back(state);

	return 0;
}

static
DWORD delete_windows(struct WinDelState *state)
{
	WaitForSingleObject(state->hTDSync, INFINITE);

	for (state->nCurrent = 0; 
		state->nCurrent < state->nTotal; state->nCurrent++) {
		Sleep(10);
	}


	return 0;
}

static void init_desktops(struct WinDelState *state)
{
	state->hOldDesk = GetThreadDesktop(GetCurrentThreadId());
	state->hNewDesk = CreateDesktopA("WinDelDesk", NULL, NULL, 0, GENERIC_ALL, NULL);
}

static void switch_desk(struct WinDelState *state)
{
	SetThreadDesktop(state->hNewDesk);
	SwitchDesktop(state->hNewDesk);
}

static void switch_back(struct WinDelState *state)
{
	SetThreadDesktop(state->hOldDesk);
	SwitchDesktop(state->hOldDesk);
	CloseDesktop(state->hNewDesk);
}
