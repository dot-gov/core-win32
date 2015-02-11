#include "utils.h"


LPVOID zalloc(__in DWORD dwSize)
{
	LPBYTE pMem = (LPBYTE) malloc(dwSize);
	RtlSecureZeroMemory(pMem, dwSize);
	return(pMem);
}

VOID zfree(__in LPVOID pMem)
{ 
	if (pMem) 
		free(pMem); 
}

void znfree(__in LPVOID* pMem)
{ 
	if(pMem == NULL)
		return;

	if(*pMem == NULL) 
		return;

	//free memory and set to null
	free(*pMem); 
	*pMem = NULL;
}


//LPWSTR version
void znfree(__in LPWSTR *pMem)
{ 
	znfree((LPVOID*)pMem);
}


//LPSTR version
void znfree(__in LPSTR *pMem)
{ 
	znfree((LPVOID*)pMem);
}

//LPBYTE version
void znfree(__in LPBYTE *pMem)
{ 
	znfree((LPVOID*)pMem);
}

//return a number aligned to uAlignment
ULONG Align(__in ULONG uSize, __in ULONG uAlignment)
{
	return (((uSize + uAlignment - 1) / uAlignment) * uAlignment);
}