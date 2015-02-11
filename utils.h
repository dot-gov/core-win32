#ifndef _UTILS_H_
#define _UTILS_H_

#include <Windows.h>

ULONG	Align(__in ULONG uSize, __in ULONG uAlignment);

LPVOID	zalloc(__in DWORD dwSize);
VOID	zfree(__in LPVOID lpMem);
void	znfree(__in LPVOID* pMem);
void	zndelete(__in LPVOID* pMem);
void	znfree(__in LPWSTR *pMem);
void	znfree(__in LPSTR *pMem);
void	znfree(__in LPBYTE *pMem);

#endif // endif