#include <windows.h>
#include <commctrl.h>

#include <stdio.h>
#include <tchar.h>
#include <stdlib.h>

#include "errmsg.h"
#include "startdlg.h"
#include "windel.h"
#include "resource.h"


int WINAPI _tWinMain(
	HINSTANCE thisInstance,
	HINSTANCE prevInstance,
	LPTSTR szCmdLine, 
	int nCmdShow
)
{
	InitCommonControls();
	ErrorMessage(thisInstance);
	StartDialog(thisInstance);

	return DeleteWindows(thisInstance);
}
