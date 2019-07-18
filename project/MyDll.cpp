#include <Windows.h>
#include <Tlhelp32.h>

#define abs(x) ((x)>0?(x):-(x))
#define pi 3.1415926535
#define MANHATTANDISTANCE(x1,y1,x2,y2) (abs((x1)-(x2))+abs((y1)-(y2)))

DWORD GetLastErrorBox(HWND hWnd, LPSTR lpTitle)
{
	LPVOID lpv;
	DWORD dwRv;

	if (GetLastError() == 0) return 0;

	dwRv = FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER |
		FORMAT_MESSAGE_FROM_SYSTEM,
		NULL,
		GetLastError(),
		MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US),
		(LPSTR)&lpv,
		0,
		NULL);

	MessageBox(hWnd, (LPCSTR)lpv, lpTitle, MB_OK);

	if (dwRv)
		LocalFree(lpv);

	SetLastError(0);
	return dwRv;
}

HMODULE fnGetProcessBase(DWORD PID)
{
	HANDLE hSnapShot;
	hSnapShot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, PID);
	if (hSnapShot == INVALID_HANDLE_VALUE)
	{
		char msg[] = "unable to create snapshot";
		GetLastErrorBox(NULL, msg);
		return NULL;
	}
	MODULEENTRY32 ModuleEntry32;
	ModuleEntry32.dwSize = sizeof(ModuleEntry32);
	if (Module32First(hSnapShot, &ModuleEntry32))
	{
		do
		{
			TCHAR szExt[5];
			strcpy(szExt, ModuleEntry32.szExePath + strlen(ModuleEntry32.szExePath) - 4);
			for (int i = 0; i < 4; i++)
			{
				if ((szExt[i] >= 'a') && (szExt[i] <= 'z'))
				{
					szExt[i] = szExt[i] - 0x20;
				}
			}
			if (!strcmp(szExt, ".EXE"))
			{
				CloseHandle(hSnapShot);
				return ModuleEntry32.hModule;
			}
		} while (Module32Next(hSnapShot, &ModuleEntry32));
	}
	CloseHandle(hSnapShot);
	return NULL;
}

void showBytes(const char* szPrefix, BYTE * p, size_t length) {
	char buffer[256];
	size_t size = strlen(szPrefix);
	memset(buffer, 0, 256);
	memcpy(buffer, szPrefix, size);
	for (size_t i = 0; i < length; ++i) {
		if (p[i] < 0x10U) {
			buffer[size + 2 * i] = '0';
			itoa(p[i], &buffer[size + 2 * i + 1], 16);
		}
		else {
			itoa(p[i], &buffer[size + 2 * i], 16);
		}
	}
	MessageBoxA(NULL, buffer, buffer, NULL);
}


void setAddress(void* _Dst, DWORD dwAddress) {
	for (int i = 0; i <= 3; ++i) {
		*(BYTE*)((int)_Dst + i) = (BYTE)(dwAddress & 0xFF);
		dwAddress >>= 8;
	}
}

HMODULE hBase;

void* p(int nOffset) {
	return (void*)((int)hBase + nOffset);
}

void target() {
	/* Red Part */
	float& theta = *(float*)p(0x6510);
	BOOL& leftPress = *(BOOL*)p(0x6500);
	BOOL& rightPress = *(BOOL*)p(0x64E4);
	if (abs(theta) >= pi/180) {
		leftPress = theta < 0;
		rightPress = theta > 0;
	}
	else {
		leftPress = rightPress = 0;
	}

	/* Yellow Part */
	int& barrierX = *(int*)p(0x64C8);
	int* barrierY = (int*)p(0x64D0);
	int& barrierDistance = *(int*)p(0x648C);
	int& barrierLength = *(int*)p(0x64B0);
	int& diamondWidth = *(int*)p(0x64C0);
	int& diamondX = *(int*)p(0x64B4);
	int& diamondY = *(int*)p(0x64C4);
	UINT& speed = *(UINT*)p(0x64B8);
	BOOL& spacePress = *(BOOL*)p(0x64E0);
	spacePress = 0;
	if (diamondX - diamondWidth <= barrierX - speed && barrierX - speed <= diamondX) {
		spacePress = 1;
	}

	/* Blue Part */
	BOOL* wall = (BOOL*)p(0x65A0);
	short& triangleOffSet = *(short*)p(0x6640);
	UINT& triangleX = *(UINT*)p(0x6644);
	UINT& wallX = *(UINT*)p(0x65DC);
	UINT& triangleWidth = *(UINT*)p(0x65D0);
	
	for (short i = 0; i <= 2; ++i) {
		if (!wall[i + 3*(wallX > triangleX + triangleWidth)]) {
			triangleOffSet = i - 1;
			break;
		}
	}

	/* Green Part */
	// have no idea how to do it automatically
	// please do it manually
}

int main() {
	DWORD dwCurrentProcessId = GetCurrentProcessId();
	hBase = fnGetProcessBase(dwCurrentProcessId);
	void* pOnTimer = p(0x16E0);

	BYTE* szOldEntry = (BYTE*)malloc(7 + 5);
	memcpy(szOldEntry, pOnTimer, 7);
	memcpy(szOldEntry + 7, "\xE9", 1);
	setAddress(szOldEntry + 8, (int)pOnTimer + 7 - ((int)szOldEntry + 12));

	BYTE* szCode = (BYTE*)malloc(5+5);
	memcpy(szCode, "\xE8", 2);  // call
	setAddress(szCode + 1, (int)target - ((int)szCode + 5));
	memcpy(szCode + 5, "\xE9", 1);
	setAddress(szCode + 6, (int)szOldEntry - ((int)szCode + 10));
	
	BYTE szNewEntry[5];
	memcpy(szNewEntry, "\xE9", 1);
	setAddress(szNewEntry + 1, (int)szCode - ((int)pOnTimer + 5));

	DWORD flOldProtect;
	VirtualProtect(pOnTimer, sizeof(szNewEntry), PAGE_EXECUTE_READWRITE, &flOldProtect);
	VirtualProtect(szCode, 10, PAGE_EXECUTE_READWRITE, &flOldProtect);
	VirtualProtect(szOldEntry, 12, PAGE_EXECUTE_READWRITE, &flOldProtect);
	
	memcpy(pOnTimer, szNewEntry, sizeof(szNewEntry));
}

BOOL WINAPI DllMain(HINSTANCE hinstDll, DWORD dwReason, LPVOID lpvRevered) {
	switch (dwReason) {
	case DLL_PROCESS_ATTACH:
		main();
		break;
	}
	return TRUE;
}