#pragma comment(linker,"\"/manifestdependency:type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

#pragma comment(lib, "wininet")

#include <windows.h>
#include <wininet.h>
#include "json11.hpp"

TCHAR szClassName[] = TEXT("Window");

int GetStroke(WCHAR c)
{
	HINTERNET hInternet = InternetOpen(NULL, INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0);
	if (hInternet == NULL)
	{
		return 0;
	}
	HINTERNET hSession = InternetConnectW(hInternet, L"mojikiban.ipa.go.jp", INTERNET_DEFAULT_HTTPS_PORT, NULL, NULL, INTERNET_SERVICE_HTTP, 0, 0);
	if (hSession == NULL)
	{
		InternetCloseHandle(hInternet);
		return 0;
	}
	WCHAR szPath[1024];
	wsprintfW(szPath, L"/mji/q?UCS=0x%x", c);
	HINTERNET hRequest = HttpOpenRequestW(hSession, L"GET", szPath, NULL, NULL, NULL, INTERNET_FLAG_NO_CACHE_WRITE | INTERNET_FLAG_SECURE, 0);
	if (hRequest == NULL)
	{
		InternetCloseHandle(hSession);
		InternetCloseHandle(hInternet);
		return 0;
	}
	if (!HttpSendRequestW(hRequest, 0, 0, 0, 0))
	{
		InternetCloseHandle(hRequest);
		InternetCloseHandle(hSession);
		InternetCloseHandle(hInternet);
		return 0;
	}
	WCHAR szBuffer[256] = { 0 };
	DWORD dwBufferSize = _countof(szBuffer);
	HttpQueryInfoW(hRequest, HTTP_QUERY_CONTENT_LENGTH, szBuffer, &dwBufferSize, NULL);
	const DWORD dwContentLength = _wtol(szBuffer);
	LPBYTE lpByte = (LPBYTE)GlobalAlloc(0, dwContentLength + 1);
	DWORD dwReadSize;
	InternetReadFile(hRequest, lpByte, dwContentLength, &dwReadSize);
	lpByte[dwReadSize] = 0;
	int nStroke = 0;
	{
		std::string parseerror;
		json11::Json json = json11::Json::parse(std::string((LPSTR)lpByte), parseerror);
		if (parseerror.size() == 0 && json["status"].string_value() == "success")
		{
			auto results = json["results"];
			if (results.is_array())
			{
				nStroke = results[0][u8"総画数"].int_value();
			}
		}
	}
	GlobalFree(lpByte);
	InternetCloseHandle(hRequest);
	InternetCloseHandle(hSession);
	InternetCloseHandle(hInternet);
	return nStroke;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	static HWND hEdit1;
	static HWND hEdit2;
	switch (msg)
	{
	case WM_CREATE:
		hEdit1 = CreateWindowEx(WS_EX_CLIENTEDGE, TEXT("EDIT"), 0, WS_VISIBLE | WS_CHILD | ES_AUTOHSCROLL, 0, 0, 0, 0, hWnd, 0, ((LPCREATESTRUCT)lParam)->hInstance, 0);
		hEdit2 = CreateWindowEx(WS_EX_CLIENTEDGE, TEXT("EDIT"), 0, WS_VISIBLE | WS_CHILD | ES_AUTOHSCROLL | ES_READONLY, 0, 0, 0, 0, hWnd, 0, ((LPCREATESTRUCT)lParam)->hInstance, 0);
		break;
	case WM_SIZE:
		MoveWindow(hEdit1, 10, 10, LOWORD(lParam) - 20, 32, TRUE);
		MoveWindow(hEdit2, 10, 50, LOWORD(lParam) - 20, 32, TRUE);
		break;
	case WM_COMMAND:
		if ((HWND)lParam == hEdit1 && HIWORD(wParam) == EN_CHANGE)
		{
			const int nSize = GetWindowTextLengthW(hEdit1);
			LPWSTR lpszText = (LPWSTR)GlobalAlloc(0, sizeof(WCHAR) * (nSize + 1));
			GetWindowTextW(hEdit1, lpszText, nSize + 1);
			int nStroke = 0;
			for (int i = 0; i < nSize; ++i)
			{
				nStroke += GetStroke(lpszText[i]);
			}
			GlobalFree(lpszText);
			TCHAR szText[1024];
			wsprintf(szText, TEXT("総画数は %d 画"), nStroke);
			SetWindowText(hEdit2, szText);
		}
		break;
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	default:
		return DefWindowProc(hWnd, msg, wParam, lParam);
	}
	return 0;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPreInst, LPSTR pCmdLine, int nCmdShow)
{
	MSG msg;
	WNDCLASS wndclass = {
		CS_HREDRAW | CS_VREDRAW,
		WndProc,
		0,
		0,
		hInstance,
		0,
		LoadCursor(0,IDC_ARROW),
		(HBRUSH)(COLOR_WINDOW + 1),
		0,
		szClassName
	};
	RegisterClass(&wndclass);
	HWND hWnd = CreateWindow(
		szClassName,
		TEXT("IPA の MJ 文字情報取得 API を使って入力された漢字の総画数を取得する"),
		WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT,
		0,
		CW_USEDEFAULT,
		0,
		0,
		0,
		hInstance,
		0
	);
	ShowWindow(hWnd, SW_SHOWDEFAULT);
	UpdateWindow(hWnd);
	while (GetMessage(&msg, 0, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
	return (int)msg.wParam;
}
