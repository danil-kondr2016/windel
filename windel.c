#include "windel.h"

#include <commctrl.h>

#include <stdio.h>
#include <tchar.h>
#include <stdlib.h>

#include "resource.h"

#define DWWM_INITDELETE 0x2000
#define DWWM_DELETEPROC 0x2001
#define DWWM_DELETEEND  0x2002
#define DWWM_REBOOT     0x2003

HWND hProgressBar, hProcessStateText, hPercents;
HWND hMainWindow;

HDESK hOldDesk, hNewDesk;

NONCLIENTMETRICS metrics = {0};

TCHAR szWindowClass[] = _T("DeletingWindow");

int cx, cy;
int width = 480, height = 300;

int counter = 0;
int delState = 0;

HINSTANCE hInstance;

static LRESULT CALLBACK windel_proc(HWND, UINT, WPARAM, LPARAM);
static ATOM register_class(HINSTANCE hThisInstance);
static HWND create_window(HINSTANCE hThisInstance);
static void init_desktops(void);
static void switch_desk(void);
static void switch_back(void);

static DWORD delete_windows(HINSTANCE hThisInstance);

static void ThisIsJustJoke(HINSTANCE hThisInstance)
{
	TCHAR szMessage[1024];

	LoadString(hThisInstance, IDS_JUST_JOKE, szMessage, 1024);
	MessageBox(NULL, szMessage, L" ", MB_ICONASTERISK);
}

int DeleteWindows(HINSTANCE hThisInstance)
{
	HANDLE hThread;
	DWORD code;

	hThread = CreateThread(
		NULL,
		0,
		delete_windows,
		hThisInstance,
		0,
		NULL);

	WaitForSingleObject(hThread, INFINITE);

	ThisIsJustJoke(hThisInstance);

	if (GetExitCodeThread(hThread, &code))
		return code;

	return 0;
}

static
DWORD delete_windows(HINSTANCE hThisInstance)
{
	MSG msg;
	
	init_desktops();
	switch_desk();

	if (!register_class(hThisInstance)) {
		MessageBox(
			NULL, _T("Failed to register DeletingWindow hWindow class"),
			NULL, MB_ICONHAND
		);
		switch_back();
		return 1;
	}

	metrics.cbSize = sizeof(NONCLIENTMETRICS);
	SystemParametersInfo(
		SPI_GETNONCLIENTMETRICS,
		sizeof(NONCLIENTMETRICS), &metrics,
		0
	);
	cx = GetSystemMetrics(SM_CXSCREEN) >> 1;
	cy = GetSystemMetrics(SM_CYSCREEN) >> 1;
	
	hMainWindow = create_window(hThisInstance);

	ShowWindow(hMainWindow, SW_SHOW);
	UpdateWindow(hMainWindow);

	while (GetMessage(&msg, NULL, 0, 0) != 0) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	Sleep(2000);
	switch_back();

	return msg.wParam;
}

static void windel_iteration(HWND hWindow)
{
	PBRANGE range;
	TCHAR text[8];

	if (delState == 2) {
		PostMessage(hWindow, DWWM_REBOOT, 0, 0);
		KillTimer(hWindow, 0);
		return;
	}

	SendMessage(hProgressBar, PBM_GETRANGE, 0, (LPARAM)&range);
	counter++;

	_sntprintf(text, 8, _T("%d%%"), (counter*100)/(range.iHigh));

	SetWindowText(hPercents, text);
	SendMessage(hProgressBar, PBM_SETPOS, counter, 0);

	if (counter >= range.iHigh) {
		delState++;
		PostMessage(hWindow, DWWM_DELETEEND, 0, 0);
		KillTimer(hWindow, 0);
	}
}

static void windel_end(HWND hWindow)
{
	TCHAR szEndMessage[1024];

	LoadString(hInstance, IDS_END_MSG, szEndMessage, 1024);
	SetWindowText(hProcessStateText, szEndMessage);

	counter = 0;

	SetTimer(hWindow, 0, 2000, NULL);
}

static void windel_create(HWND hWindow)
{
	HMENU system_menu;
	HFONT hDialogFont;
	TCHAR szDeletingMessage[1024];

	system_menu = GetSystemMenu(hWindow, FALSE);
	EnableMenuItem(system_menu, SC_CLOSE, MF_DISABLED | MF_GRAYED);

	hDialogFont = CreateFontIndirect(&metrics.lfMessageFont);

	hProgressBar = CreateWindowEx(
		0,
		PROGRESS_CLASS,
		_T(""),
		WS_CHILD | WS_VISIBLE,
		25, height-75,
		width-50, 20,
		hWindow, NULL,
		hInstance, NULL
	);
	SendMessage(hProgressBar, PBM_SETRANGE, 0, MAKELPARAM(0, 100));
	SendMessage(hProgressBar, PBM_SETSTEP, 1, 0);

	LoadString(hInstance, IDS_DELETING_MSG, szDeletingMessage, 1024);
	hProcessStateText = CreateWindowEx(
		0,
		WC_STATIC,
		szDeletingMessage,
		WS_CHILD | WS_VISIBLE | SS_LEFT,
		25, 25,
		width - 50, 100,
		hWindow, NULL,
		hInstance, NULL
	);
	SendMessage(
		hProcessStateText, WM_SETFONT,
		(WPARAM)hDialogFont, FALSE
	);

	hPercents = CreateWindowEx(
		0,
		WC_STATIC,
		_T("0%"),
		WS_CHILD | WS_VISIBLE | SS_CENTER,
		25, height-95,
		width - 50, 20,
		hWindow, NULL,
		hInstance, NULL
	);
	SendMessage(hPercents, WM_SETFONT, (WPARAM)hDialogFont, FALSE);

	SendMessage(hWindow, DWWM_INITDELETE, 0, 0);

}

static
LRESULT CALLBACK windel_proc(
	HWND hWindow,
	UINT uMessage,
	WPARAM wParam,
	LPARAM lParam
)
{
	switch (uMessage) {
	case WM_CREATE:
		windel_create(hWindow);
		return 0;
	case DWWM_INITDELETE:
		delState++;
		SendMessage(hProgressBar, PBM_SETRANGE, 0, MAKELPARAM(0, 1000));
		SetTimer(hWindow, 0, 10, NULL);
		return 0;
	case DWWM_DELETEEND:
		windel_end(hWindow);
		return 0;
	case DWWM_REBOOT:
		PostMessage(hWindow, WM_CLOSE, 0, 0);
		return 0;
	case WM_TIMER:
		windel_iteration(hWindow);
		return 0;
	case WM_DESTROY: 
		PostQuitMessage(0);
		return 0;
	}

	return DefWindowProc(hWindow, uMessage, wParam, lParam);
}

static ATOM register_class(HINSTANCE hThisInstance)
{
	WNDCLASSEX wndClass = { 0 };

	wndClass.cbSize = sizeof(WNDCLASSEX);
	wndClass.lpszClassName = szWindowClass;
	wndClass.lpszMenuName = NULL;
	wndClass.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
	wndClass.hCursor = LoadCursor(NULL, IDC_ARROW);
	wndClass.hIcon = LoadIcon(hThisInstance, MAKEINTRESOURCE(IDI_INSTALL));
	wndClass.hIconSm = LoadIcon(hThisInstance, MAKEINTRESOURCE(IDI_INSTALL));
	wndClass.lpfnWndProc = (WNDPROC)windel_proc;
	
	return RegisterClassEx(&wndClass);
}

static HWND create_window(HINSTANCE hThisInstance)
{
	TCHAR szWindowTitle[256];
	
	hInstance = hThisInstance;
	
	LoadString(hThisInstance, IDS_DELETING_TITLE, szWindowTitle, 256);
	return CreateWindowEx(
		WS_EX_TOPMOST | WS_EX_WINDOWEDGE,
		szWindowClass, szWindowTitle,
		WS_OVERLAPPEDWINDOW
	 & ~WS_THICKFRAME
	 & ~WS_MAXIMIZEBOX
	 & ~WS_MINIMIZEBOX,
		cx - (width >> 1), cy - (height >> 1) - (metrics.iCaptionHeight << 1),
		width, height,
		NULL, NULL, hThisInstance, NULL
	);
}

static void init_desktops(void)
{
	hOldDesk = GetThreadDesktop(GetCurrentThreadId());
	hNewDesk = CreateDesktopA("WinDelDesk", NULL, NULL, 0, GENERIC_ALL, NULL);
}

static void switch_desk(void)
{
	SetThreadDesktop(hNewDesk);
	SwitchDesktop(hNewDesk);
}

static void switch_back(void)
{
	SetThreadDesktop(hOldDesk);
	SwitchDesktop(hOldDesk);
	CloseDesktop(hNewDesk);
}
