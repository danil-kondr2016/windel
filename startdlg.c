#include "startdlg.h"

#include "resource.h"

static INT_PTR CALLBACK StartDeletingDlgProc(HWND, UINT, WPARAM, LPARAM);

void StartDialog(HINSTANCE hThisInstance)
{
	DialogBox(hThisInstance,
		MAKEINTRESOURCE(IDD_START),
		NULL,
		StartDeletingDlgProc);
}

static INT_PTR startdlg_init(HWND hDialog)
{
	NONCLIENTMETRICS metrics;
	HFONT hMessageFont;

	metrics.cbSize = sizeof(NONCLIENTMETRICS);
	SystemParametersInfo(
		SPI_GETNONCLIENTMETRICS,
		sizeof(NONCLIENTMETRICS), &metrics,
		0
	);

	hMessageFont = CreateFontIndirect(&metrics.lfMessageFont);
	SendDlgItemMessage(
		hDialog, 
		IDC_START_TEXT, 
		WM_SETFONT, 
		(WPARAM)hMessageFont, 
		TRUE
	);

	SetTimer(hDialog, 0, 3000, NULL);

	return TRUE;
}

static
INT_PTR CALLBACK StartDeletingDlgProc(
	HWND hDialog,
	UINT message,
	WPARAM wParam,
	LPARAM lParam
)
{
	switch (message) {
	case WM_INITDIALOG:
		return startdlg_init(hDialog);
	case WM_TIMER:
		KillTimer(hDialog, 0);
		EndDialog(hDialog, 0);
		return TRUE;
	}
	return FALSE;
}
