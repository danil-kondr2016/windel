#include "errmsg.h"

#include "resource.h"

#define MAX_N_ATTEMPTS 1
void ErrorMessage(HINSTANCE thisInstance)
{
	int answer = IDCANCEL;
	TCHAR szErrorTitle[256];
	TCHAR szErrorMessage[1024];
	TCHAR szDeclineMessage[1024];
	
	LoadString(thisInstance, IDS_ERROR_TITLE, szErrorTitle, 256);
	LoadString(thisInstance, IDS_ERROR_MSG, szErrorMessage, 1024);
	LoadString(thisInstance, IDS_DECLINE_MSG, szDeclineMessage, 1024);

	int nAttempts = 0;
	while (answer != IDYES) {
		if (nAttempts >= MAX_N_ATTEMPTS)
			break;

		answer = MessageBox(
			NULL, 
			szErrorMessage,
			szErrorTitle, 
			MB_YESNOCANCEL | MB_ICONERROR | MB_SYSTEMMODAL
		);
		if ((answer == IDCANCEL) || (answer == IDNO)) {
			nAttempts++;
			MessageBox(
			    NULL, szDeclineMessage,
			    szErrorTitle, MB_SYSTEMMODAL
			);
			continue;
		}
	}
}

