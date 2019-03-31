#include <windows.h>

BOOL WINAPI DllMain(HINSTANCE hinstDll, DWORD dwReason, LPVOID lpvRevered) {
	switch (dwReason) {
	case DLL_PROCESS_ATTACH:
		MessageBoxA(NULL, "\\(*￣v￣)/~Hi", "Hi Calculator", NULL);
		break;
	}
	return TRUE;
}
