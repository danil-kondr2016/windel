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

HWND hProgressBar, hProcessStateText, hPercents, hMainWindow;
HDESK hOldDesk, hNewDesk;
NONCLIENTMETRICS metrics = {0};

TCHAR szWindowClass[] = _T("DeletingWindow");

#define WIDTH   208
#define HEIGHT  108

#define PROCESS_STATE_TEXT_HEIGHT 40
#define PROGRESS_BAR_HEIGHT       9
#define PERCENTS_HEIGHT           10

#define LMARGIN 8
#define RMARGIN 8
#define TMARGIN 8
#define BMARGIN 8

RECT rcMargin;
COORD cProgressBar, cProcessStateText, cPercents;
SIZE sMainWindow, sProgressBar, sProcessStateText, sPercents;
SIZE sBaseUnits;

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
		(LPTHREAD_START_ROUTINE)delete_windows,
		(LPVOID)hThisInstance,
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

static void get_process_state_text_size(void);
static void get_progress_bar_size(void);
static void get_percents_size(void);

static void get_size(HWND hWindow)
{
	RECT rect;

	GetClientRect(hWindow, &rect);
	sMainWindow.cx = rect.right;
	sMainWindow.cy = rect.bottom;

	get_process_state_text_size();
	get_progress_bar_size();
	get_percents_size();
}

static void get_process_state_text_size(void)
{
	sProcessStateText.cx = sMainWindow.cx - rcMargin.left - rcMargin.right;
	sProcessStateText.cy = MulDiv(PROCESS_STATE_TEXT_HEIGHT, sBaseUnits.cy, 8);

	cProcessStateText.X = rcMargin.left;
	cProcessStateText.Y = rcMargin.top;
}

static void get_progress_bar_size(void)
{
	sProgressBar.cx = sMainWindow.cx - rcMargin.left - rcMargin.right;
	sProgressBar.cy = MulDiv(PROGRESS_BAR_HEIGHT, sBaseUnits.cy, 8);

	cProgressBar.X = rcMargin.left;
	cProgressBar.Y = sMainWindow.cy - rcMargin.right 
		- sProgressBar.cy;
}

static void get_percents_size(void)
{
	sPercents.cx = sMainWindow.cx - rcMargin.left - rcMargin.right;
	sPercents.cy = MulDiv(PERCENTS_HEIGHT, sBaseUnits.cy, 8);

	cPercents.X = rcMargin.left;
	cPercents.Y = cProgressBar.Y - sBaseUnits.cy/8 - sPercents.cy;
}

static void create_progress_bar(HWND hWindow)
{
	hProgressBar = CreateWindowEx(
		0,
		PROGRESS_CLASS,
		NULL,
		WS_CHILD | WS_VISIBLE,
		cProgressBar.X, cProgressBar.Y,
		sProgressBar.cx, sProgressBar.cy,
		hWindow, NULL,
		hInstance, NULL
	);

	SendMessage(hProgressBar, PBM_SETRANGE, 0, MAKELPARAM(0, 100));
	SendMessage(hProgressBar, PBM_SETSTEP, 1, 0);
}

static void create_progress_state_text(HWND hWindow, HFONT hDialogFont)
{
	TCHAR szDeletingMessage[1024];

	LoadString(hInstance, IDS_DELETING_MSG, szDeletingMessage, 1024);

	hProcessStateText = CreateWindowEx(
		0,
		WC_STATIC,
		szDeletingMessage,
		WS_CHILD | WS_VISIBLE | SS_LEFT,
		cProcessStateText.X,
		cProcessStateText.Y,
		sProcessStateText.cx,
		sProcessStateText.cy,
		hWindow, NULL,
		hInstance, NULL
	);
	SendMessage(
		hProcessStateText, WM_SETFONT,
		(WPARAM)hDialogFont, FALSE
	);
}

static void create_percents_text(HWND hWindow, HFONT hDialogFont)
{
	hPercents = CreateWindowEx(
		0,
		WC_STATIC,
		_T("0%"),
		WS_CHILD | WS_VISIBLE | SS_CENTER,
		cPercents.X,
		cPercents.Y,
		sPercents.cx,
		sPercents.cy,
		hWindow, NULL,
		hInstance, NULL
	);
	SendMessage(hPercents, WM_SETFONT, (WPARAM)hDialogFont, FALSE);
}

static void windel_create(HWND hWindow)
{
	HMENU system_menu;
	HFONT hDialogFont;

	get_size(hWindow);

	system_menu = GetSystemMenu(hWindow, FALSE);
	EnableMenuItem(system_menu, SC_CLOSE, MF_DISABLED | MF_GRAYED);

	hDialogFont = CreateFontIndirect(&metrics.lfMessageFont);

	create_progress_bar(hWindow);
	create_progress_state_text(hWindow, hDialogFont);
	create_percents_text(hWindow, hDialogFont);

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

static void calculate_window_size(void)
{
	LONG lBaseUnits;
	WORD wBaseX, wBaseY;

	lBaseUnits = GetDialogBaseUnits();
	wBaseX = LOWORD(lBaseUnits);
	wBaseY = HIWORD(lBaseUnits);

	rcMargin.left = MulDiv(LMARGIN, wBaseX, 4);
	rcMargin.right = MulDiv(RMARGIN, wBaseX, 4);
	rcMargin.top = MulDiv(TMARGIN, wBaseY, 8);
	rcMargin.bottom = MulDiv(BMARGIN, wBaseY, 8);

	sMainWindow.cx = MulDiv(WIDTH, wBaseX, 4);
	sMainWindow.cy = MulDiv(HEIGHT, wBaseY, 8);

	sBaseUnits.cx = wBaseX;
	sBaseUnits.cy = wBaseY;
}

static void init_metrics(void)
{
	metrics.cbSize = sizeof(NONCLIENTMETRICS);
	SystemParametersInfo(
		SPI_GETNONCLIENTMETRICS,
		sizeof(NONCLIENTMETRICS), &metrics,
		0
	);
}

static void get_center(void)
{
	cx = GetSystemMetrics(SM_CXSCREEN) >> 1;
	cy = GetSystemMetrics(SM_CYSCREEN) >> 1;
}

static HWND create_window(HINSTANCE hThisInstance)
{
	TCHAR szWindowTitle[256];

	init_metrics();
	get_center();
	calculate_window_size();

	hInstance = hThisInstance;
	
	LoadString(hThisInstance, IDS_DELETING_TITLE, szWindowTitle, 256);
	return CreateWindowEx(
		WS_EX_TOPMOST | WS_EX_WINDOWEDGE,
		szWindowClass, szWindowTitle,
		WS_OVERLAPPEDWINDOW
	      &~WS_THICKFRAME
	      &~WS_MAXIMIZEBOX
	      &~WS_MINIMIZEBOX,
		cx - (sMainWindow.cx >> 1), 
		cy - (sMainWindow.cy >> 1) - (metrics.iCaptionHeight << 1),
		sMainWindow.cx, sMainWindow.cy,
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
