#if defined(_WIN32) || defined(_WIN64)

#if !(defined(WINVER) && defined(_WIN32_WINNT))
	#include <sdkddkver.h>
	#undef WINVER
	#define WINVER _WIN32_WINNT_VISTA
	#undef _WIN32_WINNT
	#define _WIN32_WINNT _WIN32_WINNT_VISTA
#endif

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <commctrl.h>
#include <richedit.h>

#include "TextBox_Win.h"

#define TBWIN_WIDTH_MIN 180
#define TBWIN_HEIGHT_MIN 160

#ifndef STATE_SYSTEM_UNAVAILABLE
#define STATE_SYSTEM_UNAVAILABLE 0x00000001
#endif
#ifndef OBJID_VSCROLL
#define OBJID_VSCROLL ((LONG)0xFFFFFFFB)
#endif
#ifndef OBJID_HSCROLL
#define OBJID_HSCROLL ((LONG)0xFFFFFFFA)
#endif

#define ID_EDIT 40000
#define ID_OK 40001

typedef struct tagTEXTBOXPARAM{
	char *sText;
	_Bool bAutoSize;
}TEXTBOXPARAM;




////////////////////////////////////////////////////////////////////////////
//	StrConvAndAllocToWinNewline
//
//	"...\n..." -> "...\r\n..."
//	"...\r\n..." -> "...\r\n..."
//	Returns a pointer to a allocated space with the converted string
//	If no longer needed -> FreeAllocSpace()
//	opt.: zpSize => returns the length including the terminating null
//
//  Returns:
//		NULL		on failure
//
char *StrConvAndAllocToWinNewline(const char *sText, size_t *zpSize)
{
	size_t zSize;
	const char *sTextRun;
	char *sPrep, *sPrepRun;
	HANDLE hHeap;

	hHeap = GetProcessHeap();
	if(!(sText && hHeap)){
		return NULL;
	}

	zSize = 1;
	sTextRun = sText;
	while(*sTextRun != '\0'){
		if(*sTextRun == '\r' && *(sTextRun+1) == '\n'){
			++sTextRun;
		}
		if(*sTextRun++ == '\n'){
			++zSize;
		}
		++zSize;
	}

	sPrep = HeapAlloc(hHeap, 0, zSize*sizeof(*sPrep));
	if(!sPrep){
		return NULL;
	}

	sTextRun = sText;
	sPrepRun = sPrep;
	while(*sTextRun != '\0'){
		if(*sTextRun == '\r' && *(sTextRun+1) == '\n'){
			++sTextRun;
		}
		if(*sTextRun == '\n'){
			*sPrepRun++ = '\r';
		}
		*sPrepRun++ = *sTextRun++;
	}
	*sPrepRun = '\0';

	if(zpSize){
		*zpSize = zSize;
	}

	return sPrep;
}


int FreeAllocSpace(void *pvSpace)
{
	HANDLE hHeap;

	if(pvSpace){
		hHeap = GetProcessHeap();
		if(hHeap){
			return !HeapFree(hHeap, 0, pvSpace);
		}
	}

	return 1;
}


////////////////////////////////////////////////////////////////////////////
//	GetFileStringContent
//
//	Reads a file and copies it to a allocated space
//	If no longer needed -> FreeAllocSpace
//	opt.: zpSize => returns the length excluding the terminating null
//
//  Returns:
//		NULL		on failure
//
char *GetFileStringContent(const char *sFile, size_t *zpSize)
{
	HANDLE hFile, hHeap;
	char *sData = NULL;
	size_t zFileSize;
	LARGE_INTEGER liFileSize;
	DWORD dw;
	_Bool b;

	if(sFile){
		hFile = CreateFile(sFile, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
		if(hFile != INVALID_HANDLE_VALUE){
			hHeap = GetProcessHeap();
			if(hHeap && GetFileSizeEx(hFile, &liFileSize)){
				if(!liFileSize.HighPart){
					zFileSize = liFileSize.LowPart;
					sData = HeapAlloc(hHeap, 0, (zFileSize+1)*sizeof(*sData));
					if(sData){
						b = ReadFile(hFile, sData, zFileSize, &dw, NULL);
						if(b && dw == zFileSize){
							sData[zFileSize] = '\0';
							if(zpSize){
								*zpSize = zFileSize;
							}
						}
						else{
							FreeAllocSpace(sData);
							sData = NULL;
						}
					}
				}
			}
			CloseHandle(hFile);
		}
	}

	return sData;
}



static LRESULT CALLBACK TextBoxProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch(uMsg)
	{
	case WM_CREATE:{
		HWND hWndEdit, hWndOk;
		HFONT hFont;
		RECT rect;
		HDC hDC;
		int iX, iY, iCX, iCY;
		TEXTBOXPARAM *ptbParam = ((CREATESTRUCT*)lParam)->lpCreateParams;

		if(!ptbParam){
			DestroyWindow(hWnd);
			return -1;
		}

		hWndEdit = CreateWindowEx(0, WC_EDIT, ptbParam->sText, WS_VISIBLE | WS_CHILD | WS_VSCROLL | WS_HSCROLL | ES_MULTILINE | ES_READONLY,	// ES_OEMCONVERT
								0, 0, 0, 0, hWnd, (HMENU)ID_EDIT, ((CREATESTRUCT*)lParam)->hInstance, NULL);
		hWndOk = CreateWindowEx(0, WC_BUTTON, "OK", WS_VISIBLE | WS_CHILD | WS_TABSTOP | BS_PUSHBUTTON | BS_FLAT,
								0, 0, 0, 0, hWnd, (HMENU)ID_OK, ((CREATESTRUCT*)lParam)->hInstance, NULL);

		if(!(hWndEdit && hWndOk)){
			DestroyWindow(hWnd);
			return -1;
		}

		hFont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
		if(hFont){
			SendMessage(hWndEdit, WM_SETFONT, (WPARAM)hFont, MAKELPARAM(TRUE, 0));
			SendMessage(hWndOk, WM_SETFONT, (WPARAM)hFont, MAKELPARAM(TRUE, 0));
		}

		if(ptbParam->bAutoSize){
			hDC = GetDC(hWndEdit);
			if(hDC){
				if(hFont){
					SelectObject(hDC, hFont);
				}
				ZeroMemory(&rect, sizeof(rect));
				DrawText(hDC, ptbParam->sText, -1, &rect, DT_CALCRECT | DT_EXPANDTABS | DT_NOPREFIX | DT_EDITCONTROL);	// DT_EXTERNALLEADING
				ReleaseDC(hWndEdit, hDC);
				AdjustWindowRectEx(&rect, ((CREATESTRUCT*)lParam)->style, FALSE, ((CREATESTRUCT*)lParam)->dwExStyle);
				iCX = GetSystemMetrics(SM_CXSCREEN)?:140;
				iCY = GetSystemMetrics(SM_CYSCREEN)?:140;
				iX = rect.right-rect.left < iCX - 120 ? rect.right-rect.left : iCX - 120;
				iY = rect.bottom-rect.top < iCY - 120 ? rect.bottom-rect.top : iCY - 120;
				MoveWindow(hWnd, (iCX-iX)>>1, (iCY-iY)>>1, iX + 44, iY + 56, TRUE);
			}
		}

	}return 0;
	//case WM_CTLCOLORSTATIC:{
		//HDC hdcStatic = (HDC)wParam;
		//SetTextColor(hdcStatic, RGB(0,0,0));
		//SetTextColor(hdcStatic, RGB(255,255,255));
		//SetBkColor(hdcStatic, RGB(0,0,0));
	//}return (LRESULT)GetStockObject(WHITE_BRUSH);
	case WM_COMMAND:
		switch(LOWORD(wParam)){
		case ID_OK:
			DestroyWindow(hWnd);
			break;
		}
		break;
	case WM_SIZE:{
		HWND hWndEdit, hWndOk;
		SCROLLBARINFO sbInfo;

		hWndEdit = GetDlgItem(hWnd, ID_EDIT);
		hWndOk = GetDlgItem(hWnd, ID_OK);

		if(!(hWndEdit && hWndOk)){
			DestroyWindow(hWnd);
			return 0;
		}

		ShowScrollBar(hWndEdit, SB_BOTH, TRUE);
		MoveWindow(hWndEdit, 8, 4, LOWORD(lParam)-16, HIWORD(lParam)-38, TRUE);
		MoveWindow(hWndOk, LOWORD(lParam)-90, HIWORD(lParam)-28, 86, 22, TRUE);

		ZeroMemory(&sbInfo, sizeof(sbInfo));
		sbInfo.cbSize = sizeof(sbInfo);
		if(GetScrollBarInfo(hWndEdit, OBJID_HSCROLL, &sbInfo)){
			ShowScrollBar(hWndEdit, SB_HORZ, (sbInfo.rgstate[0] & STATE_SYSTEM_UNAVAILABLE ? FALSE : TRUE));
		}
		ZeroMemory(&sbInfo, sizeof(sbInfo));
		sbInfo.cbSize = sizeof(sbInfo);
		if(GetScrollBarInfo(hWndEdit, OBJID_VSCROLL, &sbInfo)){
			ShowScrollBar(hWndEdit, SB_VERT, (sbInfo.rgstate[0] &  STATE_SYSTEM_UNAVAILABLE ? FALSE : TRUE));
		}
	}break;
    case WM_DESTROY:
    	PostQuitMessage(0);
    	return 0;
	}

	return DefWindowProc(hWnd, uMsg, wParam, lParam);
}


////////////////////////////////////////////////////////////////////////////
//
//	TextBox
//
//	iWidth < TBWIN_WIDTH_MIN || iHeight < TBWIN_HEIGHT_MIN => auto-sized window
//
//  Returns:
//		> 0		on failure
//		0		on success
//
int TextBox(const char *sTitel, const char *sText, int iWidth, int iHeight)
{
	HWND hWndD;
	char *sPrep;
	int iX, iY;
	MSG msg;
	ATOM atomClassID;
	DWORD dwStyle, dwExStyle;
	WNDCLASSEX wcex;
	RECT rect;
	TEXTBOXPARAM tbParam;

	if(!sText){
		return __LINE__;
	}

	sPrep = StrConvAndAllocToWinNewline(sText, NULL);
	if(!sPrep){
		return __LINE__;
	}

	InitCommonControls();

	ZeroMemory(&wcex, sizeof(wcex));
	wcex.cbSize = sizeof(wcex);
	wcex.style = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = TextBoxProc;
	wcex.cbClsExtra = sizeof(&tbParam);
	wcex.hInstance = NULL;	//(HINSTANCE)GetModuleHandle(NULL);
	wcex.hIcon = LoadIcon(NULL, IDI_APPLICATION);
	wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);	//(HBRUSH)(COLOR_3DFACE + 1) GetSysColorBrush(COLOR_3DFACE) (HBRUSH)GetStockObject(LTGRAY_BRUSH) (HBRUSH)(COLOR_WINDOW + 1)
	wcex.lpszClassName = "TextBox";

	atomClassID = RegisterClassEx(&wcex);
	if(!atomClassID){
		return __LINE__;
	}

	ZeroMemory(&rect, sizeof(rect));
	ZeroMemory(&tbParam, sizeof(tbParam));
	dwStyle = WS_CLIPSIBLINGS | WS_CLIPCHILDREN | WS_OVERLAPPEDWINDOW | WS_VISIBLE;
	dwExStyle = WS_EX_APPWINDOW | WS_EX_WINDOWEDGE;
	tbParam.sText = sPrep;;
	if( iWidth < TBWIN_WIDTH_MIN || iHeight < TBWIN_HEIGHT_MIN){	// GetSystemMetrics(SM_CXMINTRACK)  SM_CYMINTRACK
		tbParam.bAutoSize = 1;
	}
	else{
		SetRect(&rect, 0, 0, iWidth, iHeight);
		AdjustWindowRectEx(&rect, dwStyle, FALSE, dwExStyle);
	}

	iX = (GetSystemMetrics(SM_CXSCREEN)-(rect.right-rect.left))>>1;
	if(iX < 1){
		iX = CW_USEDEFAULT;
	}
	iY = (GetSystemMetrics(SM_CYSCREEN)-(rect.bottom-rect.top))>>1;
	if(iY < 1){
		iY = CW_USEDEFAULT;
	}
	hWndD = CreateWindowEx( dwExStyle, MAKEINTATOM(atomClassID), sTitel, dwStyle,
							iX, iY, rect.right-rect.left, rect.bottom-rect.top,
							NULL, NULL, wcex.hInstance, &tbParam);

	if(FreeAllocSpace(sPrep) || !hWndD){
		UnregisterClass(MAKEINTATOM(atomClassID), wcex.hInstance);
		return __LINE__;
	}

	ShowWindow(hWndD, SW_SHOW);
	UpdateWindow(hWndD);
	SetForegroundWindow(hWndD);
	SetFocus(hWndD);

	while(GetMessage(&msg, NULL, 0, 0) > 0){
		if(!IsDialogMessage(hWndD, &msg)){
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	return !UnregisterClass(MAKEINTATOM(atomClassID), wcex.hInstance);
}

#endif
