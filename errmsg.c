#include "errmsg.h"

#include "resource.h"

#include <commctrl.h>

#define MAX_N_ATTEMPTS 1
#define TIMEOUT 10000

static
HRESULT WINAPI
ErrorMessageTaskDialogCallback(
	HWND hWnd,
	UINT uMsg,
	WPARAM wParam,
	LPARAM lParam,
	LONG_PTR lpRefData
)
{
	switch (uMsg) {
	case TDN_BUTTON_CLICKED:
		if (wParam == IDCLOSE || wParam == IDNO) {
			SendMessage(hWnd, TDM_ENABLE_BUTTON, IDNO, 0);
			SendMessage(hWnd, TDM_ENABLE_BUTTON, IDCLOSE, 0);
			SendMessage(hWnd, TDM_SET_ELEMENT_TEXT, TDE_CONTENT,
				(LPARAM)MAKEINTRESOURCE(IDS_DECLINE_MSG));
			return S_FALSE;
		}
		else
			return S_OK;
		break;
	case TDN_TIMER:
		if (wParam >= TIMEOUT) {
			EndDialog(hWnd, 0);
		}
		return S_OK;
	}

	return S_FALSE;
}

void ErrorMessage(HINSTANCE thisInstance)
{
	int answer = IDCANCEL;
	TASKDIALOGCONFIG tdcfg = {0};

	tdcfg.cbSize = sizeof(tdcfg);
	tdcfg.hInstance = thisInstance;
	tdcfg.dwFlags = TDF_CALLBACK_TIMER;
	tdcfg.dwCommonButtons = TDCBF_YES_BUTTON | TDCBF_NO_BUTTON;
	tdcfg.pszWindowTitle = MAKEINTRESOURCE(IDS_ERROR_TITLE);
	tdcfg.pszMainInstruction = MAKEINTRESOURCE(IDS_ERROR_MAIN_INSTR);
	tdcfg.pszContent = MAKEINTRESOURCE(IDS_ERROR_MSG);
	tdcfg.pfCallback = ErrorMessageTaskDialogCallback;
	tdcfg.pszMainIcon = MAKEINTRESOURCE(TD_ERROR_ICON);
	
	TaskDialogIndirect(&tdcfg, NULL, NULL, NULL);
}

