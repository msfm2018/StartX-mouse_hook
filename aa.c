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
HWND hWndTForm1 = NULL;  // 用于保存 TForm1 窗口句柄
bool isStartBtn = false;
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

// 判断鼠标点击的位置是否在TForm1窗口内
BOOL IsClickInsideTForm1(POINT pt)
{
	// 获取TForm1窗口句柄
	hWndTForm1 = FindWindow(L"TForm1", L"startx");
	if (hWndTForm1 != NULL) {
		RECT rect;
		GetWindowRect(hWndTForm1, &rect);
		return (pt.x >= rect.left && pt.x <= rect.right && pt.y >= rect.top && pt.y <= rect.bottom);
	}
	return FALSE;
}

void PrintToConsole(const char* message) {
	HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
	if (hConsole == INVALID_HANDLE_VALUE) {
		AllocConsole(); // 分配一个新的控制台
		hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
	}

	DWORD written;
	WriteConsoleA(hConsole, message, strlen(message), &written, NULL);
}

void LogMessageW(const wchar_t* message) {
	// 使用宽字符写入日志文件
	FILE* logFile = _wfopen(L"c:\\12\\log.txt", L"a, ccs=UTF-8"); // 指定 UTF-8 编码
	if (logFile) {
		fwprintf(logFile, L"%s\n", message);
		fclose(logFile);
	}
	else {
		OutputDebugString(L"Failed to open log file\n");
	}
}

void LogBSTR(BSTR bstr) {
	if (bstr == NULL) {
		LogMessageW(L"BSTR is NULL");
		return;
	}
	LogMessageW(bstr);
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
		//if (elemName && elemType) {
		//	bRet = !wcscmp(elemType, L"SystemTray.OmniButton");
		//	isStartBtn = false;
		//}

		//if (elemName && elemType) {
		//	bRet = !wcscmp(elemType, L"ToggleButton");//ToggleButton开始
		//	isStartBtn = true;
		//}

		isStartBtn = false; // This indicates it's not the Start button
		if (elemName && elemType)
		{
			// Check if the element is a SystemTray.OmniButton
			if ((!wcscmp(elemType, L"SystemTray.OmniButton")) || (!wcscmp(elemType, L"SystemTray.OmniButtonLeft"))) //SystemTray.OmniButtonLeft
			{
				bRet = TRUE;
				isStartBtn = false; // This indicates it's not the Start button
			}
			// Check if the element is a ToggleButton (Start button)
			else if ((!wcscmp(elemType, L"ToggleButton")) && (!wcscmp(elemName, L"开始")))
			{

				bRet = TRUE;
				isStartBtn = true; // This indicates it's the Start button


			}

		}

		//LogBSTR(elemType);
		//LogBSTR(elemName);
		//OutputDebugStringW(L"Element Type: ");
		////SystemTray.OmniButton
		//OutputDebugStringW(elemType);
		//OutputDebugStringW(elemName);
		//OutputDebugStringW(L"\n");
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
// // 钩子回调函数
LRESULT CALLBACK MouseHookProc(int nCode, WPARAM wParam, LPARAM lParam)
{
	if (nCode >= 0)
	{
		MSLLHOOKSTRUCT* pMouse = (MSLLHOOKSTRUCT*)lParam;
		if (wParam == WM_LBUTTONDOWN)
		{
			POINT pt = pMouse->pt;

			// 如果点击的位置不在TForm1窗口内，发送 WM_CLOSE 消息
			if (!IsClickInsideTForm1(pt)) {
				if (hWndTForm1 != NULL) {
					PostMessage(hWndTForm1, WM_CLOSE, 0, 0);
				}
			}

			//else
			if (IsPointOnEmptyAreaOfNewTaskbar(pt)) {
				if (isStartBtn) {

					// Try to find the TForm1 window with the title "selfdefinestartmenu"
					HWND hwnd = FindWindow(L"TbottomForm", L"selfdefinestartmenu");
					if (hwnd != NULL)
					{
						auto hwnd1 = FindWindow(L"TForm1", L"myxyzabc");
						// Window found, you can perform further actions or send a message
						PostMessage(hwnd1, WM_USER + 1031, 0, 0);
					}
					else
					{
						// Window not found, handle accordingly
						OutputDebugStringW(L"TForm1 with title 'selfdefinestartmenu' not found!\n");
					}


				}
				else {
					auto hwnd = FindWindow(L"TForm1", L"myxyzabc");
					if (hwnd != NULL) {
						PostMessage(hwnd, WM_MY_CUSTOM_MESSAGE, 0, 0);
						return 1; // 阻止事件继续传播
					}
				}

				return 1; // 阻止事件继续传播
			}

		}
		else if (wParam == WM_RBUTTONDOWN)
		{
			POINT pt = pMouse->pt;

			// 如果点击的位置不在TForm1窗口内，发送 WM_CLOSE 消息
			if (!IsClickInsideTForm1(pt)) {
				if (hWndTForm1 != NULL) {
					PostMessage(hWndTForm1, WM_CLOSE, 0, 0);
				}
			}


		}





	}
	return CallNextHookEx(g_hMouseHook, nCode, wParam, lParam);
}
//LRESULT CALLBACK MouseHookProc(int nCode, WPARAM wParam, LPARAM lParam)
//{
//	if (nCode >= 0)
//	{
//		MSLLHOOKSTRUCT* pMouse = (MSLLHOOKSTRUCT*)lParam;
//		if (wParam == WM_LBUTTONDOWN &&
//			IsPointOnEmptyAreaOfNewTaskbar(((MOUSEHOOKSTRUCT*)lParam)->pt))
//		{
//
//			auto hwnd = FindWindow(L"TForm1", L"myxyzabc");
//			if (hwnd != NULL) {
//				PostMessage(hwnd, WM_MY_CUSTOM_MESSAGE, 0, 0);
//			}
//
//			return 1; // 返回非零值阻止事件继续传播
//		}
//
//	}
//	return CallNextHookEx(g_hMouseHook, nCode, wParam, lParam);
//}

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
