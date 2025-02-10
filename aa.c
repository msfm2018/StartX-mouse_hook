#include <initguid.h>
#include <stdio.h>
#include <stdbool.h>
#include <Windows.h>

#include <UIAutomationClient.h>
#include <math.h>
#define MAX_LOADSTRING 100
#define WM_SYSDATE_MESSAGE (WM_USER + 1030)
HHOOK g_hMouseHook = NULL;
HHOOK Shell_TrayWndMouseHook = NULL;
HWND hTrayWnd = NULL;
HWND hWndTForm1 = NULL;  // 用于保存 TForm1 窗口句柄



HHOOK g_hKeyboardHook = NULL;
HWND hWndTFormtip = NULL;  // TFormTip 的窗口句柄
HWND hWndTForm3D = NULL;  // TFormTip 的窗口句柄
HWND hWndUnderCursor = NULL;  // 当前鼠标下的窗口句柄


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





LRESULT CALLBACK KeyboardProc(int nCode, WPARAM wParam, LPARAM lParam) {
	if (nCode == HC_ACTION) {
		KBDLLHOOKSTRUCT* pKbdHook = (KBDLLHOOKSTRUCT*)lParam;

		// If Backspace or Delete key is pressed
		if (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN) {
			if (pKbdHook->vkCode == VK_BACK || pKbdHook->vkCode == VK_DELETE) {

				hWndTFormtip = FindWindow(L"FLUTTER_RUNNER_WIN32_WINDOW", L"clipform");
				if (hWndTFormtip != NULL) {


					// Get the current mouse cursor position
					POINT cursorPos;
					GetCursorPos(&cursorPos);

					// Get the window under the cursor
					HWND hWndUnderCursor = WindowFromPoint(cursorPos);

					// Optionally: Ensure that the window under the cursor is focused
					SetForegroundWindow(hWndUnderCursor);

					// Simulate the Backspace or Delete key press
					INPUT input[2] = { 0 };

					if (pKbdHook->vkCode == VK_BACK) {
						// Simulate Backspace key press and release
						input[0].type = INPUT_KEYBOARD;
						input[0].ki.wVk = VK_BACK;

						input[1].type = INPUT_KEYBOARD;
						input[1].ki.wVk = VK_BACK;
						input[1].ki.dwFlags = KEYEVENTF_KEYUP;
					}
					else if (pKbdHook->vkCode == VK_DELETE) {
						// Simulate Delete key press and release
						input[0].type = INPUT_KEYBOARD;
						input[0].ki.wVk = VK_DELETE;

						input[1].type = INPUT_KEYBOARD;
						input[1].ki.wVk = VK_DELETE;
						input[1].ki.dwFlags = KEYEVENTF_KEYUP;
					}

					// Send the input events to simulate the deletion
					SendInput(2, input, sizeof(INPUT));

					// Optionally: Send a message to the window to delete the selected text, or invoke custom logic
					// Example: Send a custom message to the window to handle the text deletion

					// For now, this simulates the deletion on the window under the cursor
					return 1;  // Stop the key event from propagating further
				}
			}
		}
	}
	return CallNextHookEx(g_hKeyboardHook, nCode, wParam, lParam);
}


// 安装低级键盘钩子
void SetKeyboardHook() {
	g_hKeyboardHook = SetWindowsHookEx(WH_KEYBOARD_LL, KeyboardProc, NULL, 0);
}

// 移除低级键盘钩子
void RemoveKeyboardHook() {
	if (g_hKeyboardHook != NULL) {
		UnhookWindowsHookEx(g_hKeyboardHook);
		g_hKeyboardHook = NULL;
	}
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


BOOL IsClickInsideTip(POINT pt)
{
	// 获取TForm1窗口句柄
	hWndTFormtip = FindWindow(L"FLUTTER_RUNNER_WIN32_WINDOW", L"clipform");
	if (hWndTFormtip != NULL) {
		RECT rect;
		GetWindowRect(hWndTFormtip, &rect);
		return (pt.x >= rect.left && pt.x <= rect.right && pt.y >= rect.top && pt.y <= rect.bottom);
	}
	return FALSE;
}

BOOL IsClickInside3D(POINT pt)
{
	// 获取TForm1窗口句柄
	hWndTForm3D = FindWindow(L"weather_class", L"weather");
	if (hWndTForm3D != NULL) {
		RECT rect;
		GetWindowRect(hWndTForm3D, &rect);
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
// 模拟 Ctrl+C
void SimulateCtrlC() {
	INPUT inputs[4] = { 0 };

	// 按下 Ctrl
	inputs[0].type = INPUT_KEYBOARD;
	inputs[0].ki.wVk = VK_CONTROL;

	// 按下 C
	inputs[1].type = INPUT_KEYBOARD;
	inputs[1].ki.wVk = 'C';

	// 释放 C
	inputs[2].type = INPUT_KEYBOARD;
	inputs[2].ki.wVk = 'C';
	inputs[2].ki.dwFlags = KEYEVENTF_KEYUP;

	// 释放 Ctrl
	inputs[3].type = INPUT_KEYBOARD;
	inputs[3].ki.wVk = VK_CONTROL;
	inputs[3].ki.dwFlags = KEYEVENTF_KEYUP;

	SendInput(4, inputs, sizeof(INPUT));
}

// 获取剪贴板文本
int GetClipboardText(wchar_t* outText, int maxLen) {
	for (int i = 0; i < 5; i++) {
		if (OpenClipboard(NULL)) {
			HANDLE hData = GetClipboardData(CF_UNICODETEXT);
			if (hData) {
				wchar_t* pText = (wchar_t*)GlobalLock(hData);
				if (pText) {
					wcsncpy_s(outText, maxLen, pText, _TRUNCATE);
					GlobalUnlock(hData);
					CloseClipboard();
					return 1;
				}
			}
			CloseClipboard();
		}
		Sleep(50);
	}
	return 0;
}

// 钩子回调函数
// // 钩子回调函数
LRESULT CALLBACK MouseHookProc(int nCode, WPARAM wParam, LPARAM lParam)
{
	static wchar_t lastText[1024] = L"";
	if (nCode >= 0)
	{
		MSLLHOOKSTRUCT* pMouse = (MSLLHOOKSTRUCT*)lParam;
		static POINT startPoint = { 0, 0 };
		if (wParam == WM_LBUTTONDOWN)
		{
			POINT pt = pMouse->pt;
			startPoint = pMouse->pt;

			// 如果点击的位置不在TForm1窗口内，发送 WM_CLOSE 消息
			if (!IsClickInsideTForm1(pt)) {
				if (hWndTForm1 != NULL) {
					PostMessage(hWndTForm1, WM_CLOSE, 0, 0);
				}
			}

			if (!IsClickInside3D(pt)) {  //&& IsWindowVisible(hWndTForm3D)
				if (hWndTForm3D != NULL && IsWindowVisible(hWndTForm3D)) {

					ShowWindow(hWndTForm3D, SW_HIDE);
				}
			}


			if (!IsClickInsideTip(pt)) {
				if (hWndTFormtip != NULL) {
					PostMessage(hWndTFormtip, WM_CLOSE, 0, 0);
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
						PostMessage(hwnd, WM_SYSDATE_MESSAGE, 0, 0);
						return 1; // 阻止事件继续传播
					}
				}

				return 1; // 阻止事件继续传播
			}

		}
		else if (wParam == WM_LBUTTONUP)
		{
			POINT endPoint = pMouse->pt;

			// 比较起始点和结束点
			if (startPoint.x == endPoint.x && startPoint.y == endPoint.y)
			{
				// 鼠标没有移动，可能是选择了文本
				OutputDebugString(L"Mouse click detected, possibly text selection\n");

				// 你可以在这里进一步处理或获取选中的文本
				// 例如，通过剪贴板或 UI 自动化检查文本是否选中
			}
			else
			{
				//SimulateCtrlC();

				//Sleep(50);

				//wchar_t currentText[1024] = L"";
				//if (GetClipboardText(currentText, 1024)) {
				//	if (wcslen(currentText) > 0 && wcsstr(currentText, L"http") == NULL) {
				//		if (wcscmp(lastText, currentText) != 0) {

				//			POINT pt = pMouse->pt;
				//			hWndUnderCursor = WindowFromPoint(pt);

				//			auto hwnd1 = FindWindow(L"TForm1", L"myxyzabc");
				//			// 告诉主程序 启动剪贴程序
				//			PostMessage(hwnd1, WM_USER + 1041, 0, 0);
				//			wcscpy_s(lastText, 1024, currentText);
				//		}
				//	}
				//}




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
		SetKeyboardHook();
	}

}

// 卸载钩子
__declspec(dllexport) void UninstallMouseHook()
{
	if (g_hMouseHook)
	{
		UnhookWindowsHookEx(g_hMouseHook);
		RemoveKeyboardHook();
		g_hMouseHook = NULL;
	}
}
