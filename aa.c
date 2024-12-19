#include <initguid.h>
#include <stdio.h>
#include <stdbool.h>
#include <Windows.h>

#include <UIAutomationClient.h>
#include <math.h>
#define MAX_LOADSTRING 100
#define WM_MY_CUSTOM_MESSAGE (WM_USER + 1030)
HHOOK g_hMouseHook = NULL;
HHOOK Shell_TrayWndMouseHook = NULL;
HWND hTrayWnd = NULL;

//钩住鼠标消息 查看点击位置 然后屏蔽这个消息 给主窗口发送 消息
typedef enum {
	Unsupported,
	Win10,
	Win11,
	Win11_22H2,
	Win11_24H2,
} WinVersion;

WinVersion g_winVersion;

typedef LONG(WINAPI* RtlGetVersionFunc)(OSVERSIONINFOEXW*);


WinVersion GetOSVersion1() {
	OSVERSIONINFOEXW osvi = { sizeof(OSVERSIONINFOEXW) };
	HMODULE hNtdll = GetModuleHandleW(L"ntdll.dll");

	if (hNtdll) {
		RtlGetVersionFunc RtlGetVersion = (RtlGetVersionFunc)GetProcAddress(hNtdll, "RtlGetVersion");
		if (RtlGetVersion && RtlGetVersion(&osvi) == 0) {
			wprintf(L"Version: %u.%u.%u\n", osvi.dwMajorVersion, osvi.dwMinorVersion, osvi.dwBuildNumber);

			switch (osvi.dwMajorVersion) {
			case 10:
				if (osvi.dwBuildNumber < 22000) {
					return Win10;
				}
				else if (osvi.dwBuildNumber <= 22000) {
					return Win11;
				}
				else if (osvi.dwBuildNumber < 26100) {
					return Win11_22H2;
				}
				else {
					return Win11_24H2;
				}
				break;
			}
		}
	}
	return Unsupported;
}
inline BOOL IsWindows11Version22H2OrHigher()
{
	return true;
}
BOOL IsPointOnEmptyAreaOfNewTaskbar(POINT pt)
{
	HRESULT hr = S_OK;
	IUIAutomation2* pIUIAutomation2 = NULL;
	IUIAutomationElement* pIUIAutomationElement = NULL;
	HWND hWnd = NULL;
	BOOL bRet = FALSE;
	BSTR elemName = NULL;
	BSTR elemType = NULL;
	BOOL bIsWindows11Version22H2OrHigher = IsWindows11Version22H2OrHigher();

	if (SUCCEEDED(hr))
	{
		hr = CoCreateInstance(&CLSID_CUIAutomation8, NULL, CLSCTX_INPROC_SERVER, &IID_IUIAutomation2, &pIUIAutomation2);
	}
	if (SUCCEEDED(hr))
	{
		hr = pIUIAutomation2->lpVtbl->ElementFromPoint(pIUIAutomation2, pt, &pIUIAutomationElement);
	}
	if (SUCCEEDED(hr) && bIsWindows11Version22H2OrHigher)
	{
		hr = pIUIAutomationElement->lpVtbl->get_CurrentName(pIUIAutomationElement, &elemName);
	}
	if (SUCCEEDED(hr) && bIsWindows11Version22H2OrHigher)
	{
		hr = pIUIAutomationElement->lpVtbl->get_CurrentClassName(pIUIAutomationElement, &elemType);
	}
	if (SUCCEEDED(hr) && bIsWindows11Version22H2OrHigher)
	{
		//bRet = elemName && elemType && (!wcscmp(elemName, L"") && wcscmp(elemType, L"SystemTray.OmniButton"));&& (!wcscmp(elemName, L""))
		if (elemName && elemType) {
			bRet = !wcscmp(elemType, L"SystemTray.OmniButton");

		}
		OutputDebugStringW(L"Element Type: ");
		//SystemTray.OmniButton
		OutputDebugStringW(elemType);
		OutputDebugStringW(elemName);
		OutputDebugStringW(L"\n");
	}


	if (elemName)
	{
		SysFreeString(elemName);
	}
	if (elemType)
	{
		SysFreeString(elemType);
	}
	if (pIUIAutomationElement)
	{
		pIUIAutomationElement->lpVtbl->Release(pIUIAutomationElement);
	}
	if (pIUIAutomation2)
	{
		pIUIAutomation2->lpVtbl->Release(pIUIAutomation2);
	}
	return bRet;
}

// 钩子回调函数
LRESULT CALLBACK MouseHookProc(int nCode, WPARAM wParam, LPARAM lParam)
{
	if (nCode >= 0)
	{
		MSLLHOOKSTRUCT* pMouse = (MSLLHOOKSTRUCT*)lParam;
		if (wParam == WM_LBUTTONDOWN &&
			IsPointOnEmptyAreaOfNewTaskbar(((MOUSEHOOKSTRUCT*)lParam)->pt))
		{

			auto hwnd = FindWindow(L"TForm1", L"myxyzabc");
			if (hwnd != NULL) {
				PostMessage(hwnd, WM_MY_CUSTOM_MESSAGE, 0, 0);
			}

			return 1; // 返回非零值阻止事件继续传播
		}

	}
	return CallNextHookEx(g_hMouseHook, nCode, wParam, lParam);
}

// 安装钩子
__declspec(dllexport) void InstallMouseHook()
{

	g_winVersion = GetOSVersion1();
	if (g_winVersion >= Win11) {
		g_hMouseHook = SetWindowsHookExW(WH_MOUSE_LL, MouseHookProc, 0, 0);
	}
	
}

// 卸载钩子
__declspec(dllexport) void UninstallMouseHook()
{
	if (g_hMouseHook)
	{
		UnhookWindowsHookEx(g_hMouseHook);
		g_hMouseHook = NULL;
	}
}
