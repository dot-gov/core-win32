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

/* these want be a secure alloc/free wrappers with the following characteristics
  
   alloc:
   - unsigned int for size parameter
   - check against max size allocation: TODO define max size
   - check against zero bytes allocation
   - zero out allocated memory

   free:
   - check against NULL before freeing
   - assign NULL to ptr after freeing


   clients:
   - check return values against NULL
   - cast immediately the returned type, i.e.
		#define MALLOC(type) ((type) malloc(sizeof(type)))
		#define MALLOC_ARRAY(number, type) ((type*) malloc((number) * sizeof(type)))

	N.B.: feel free to add any additional secure oriented allocation feature and document it 
*/

LPVOID zalloc_s(__in size_t dwSize )
{
		
	if (dwSize == 0)
		return NULL;

	LPBYTE pMem = (LPBYTE) malloc(dwSize);
	SecureZeroMemory(pMem, dwSize);

	return pMem;
}

VOID zfree_s(__in LPVOID pMem)
{
	if (pMem)
	{
		free(pMem);
		pMem = NULL;
	}

}

/* time conversion */
void UnixTimeToFileTime(time_t t, LPFILETIME pft)
{
	// Note that LONGLONG is a 64-bit value
	LONGLONG ll;

	ll = Int32x32To64(t, 10000000) + 116444736000000000;
	pft->dwLowDateTime = (DWORD)ll;
	pft->dwHighDateTime = ll >> 32;
}

void UnixTimeToSystemTime(time_t t, LPSYSTEMTIME pst)
{
	FILETIME ft;

	UnixTimeToFileTime(t, &ft);
	FileTimeToSystemTime(&ft, pst);
}