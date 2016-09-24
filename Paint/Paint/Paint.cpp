#include "stdafx.h"
#include "Paint.h"
#include <windowsx.h>
#include <commdlg.h>

#define MAX_LOADSTRING 100

HINSTANCE hInst;								// handle of the application instance
TCHAR szTitle[MAX_LOADSTRING];					// title
TCHAR szWindowClass[MAX_LOADSTRING];			// main window's class name

// The following functions are included in this module
ATOM				MyRegisterClass(HINSTANCE hInstance);
BOOL				InitInstance(HINSTANCE, int);
LRESULT CALLBACK	WndProc(HWND, UINT, WPARAM, LPARAM);
//COLORREF ShowColorDialog(HWND, COLORREF);
INT_PTR CALLBACK	About(HWND, UINT, WPARAM, LPARAM);


HPEN hPen = NULL;
HBRUSH hBrush = NULL;
BOOL fDraw = FALSE;
POINT ptPrevious = { 0 };
CHOOSECOLOR colorDlg; // struct of standart color dialog
COLORREF colors[16]; //palette
static DWORD rgbCurrent;
static HMENU menuBar;
static int penWidth = 1;


int APIENTRY _tWinMain(_In_ HINSTANCE hInstance, // application instance descriptor	
                     _In_opt_ HINSTANCE hPrevInstance, //this parameter is obsolete
                     _In_ LPTSTR    lpCmdLine,  //command line handle, this paramter is obsolete
                     _In_ int       nCmdShow)	//window's state in the initial demonstration
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

	MSG msg;
	HACCEL hAccelTable; //accelarator table descriptor

	// Global strings initialization
	LoadString(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
	LoadString(hInstance, IDC_PAINT, szWindowClass, MAX_LOADSTRING);
	MyRegisterClass(hInstance);

	// Perform application initialization
	if (!InitInstance (hInstance, nCmdShow))
	{
		return FALSE;
	}

	hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_PAINT));

	// message waiting and processing loop
	while (GetMessage(&msg, NULL, 0, 0))
	{
		if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	return (int) msg.wParam;
}

//
//  FUNCTION: MyRegisterClass()
//
//  PURPOSE: registrates window class.
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
	WNDCLASSEX wcex;

	wcex.cbSize = sizeof(WNDCLASSEX);

	//registration window's attributes
	wcex.style			= CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc	= WndProc;
	wcex.cbClsExtra		= 0;
	wcex.cbWndExtra		= 0;
	wcex.hInstance		= hInstance;
	wcex.hIcon			= LoadIcon(hInstance, MAKEINTRESOURCE(IDI_PAINT));
	wcex.hCursor		= LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground	= (HBRUSH)(COLOR_WINDOW+1);
	wcex.lpszMenuName	= MAKEINTRESOURCE(IDC_PAINT);
	wcex.lpszClassName	= szWindowClass;
	wcex.hIconSm		= LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

	return RegisterClassEx(&wcex);
}

//
//   FUNCTION: InitInstance(HINSTANCE, int)
//
//   PURPOSE: It retains instance processing and creates the main window
//
//   COMMENTS:
//
//       In this function, the instance handle is stored in a global variable, and also
//        program's main window is created and displayed
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
   HWND hWnd; //window's descriptor

   hInst = hInstance; // saving instance descriptor in gloabal variable

   hWnd = CreateWindow(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,		
      CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, NULL, NULL, hInstance, NULL);

   if (!hWnd)
   {
      return FALSE;
   }

   ShowWindow(hWnd, nCmdShow);
   UpdateWindow(hWnd);

   return TRUE;
}

//
//  FUNCTION: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  PURPOSE:  handles messages in main window
//
//  WM_COMMAND	- message about choice menu tab
//  WM_PAINT	- message about the need to redraw the client area of the window
//  WM_DESTROY	 - message about exit and return
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	int wmId, wmEvent;
	PAINTSTRUCT ps;	//contains information that can be used to paint the client area of a window owned by that application
	HDC hdc; // = GetDC(hWnd); // getting handle device context

	BOOL bRet = FALSE;
	BOOL bCmd = FALSE;

	switch (message)
	{
	/*case WM_CREATE:
		menuBar = LoadMenu(NULL, MAKEINTRESOURCE(ID_PAINT));
		SetMenu(hWnd, menuBar);
		break;*/
	case WM_INITDIALOG:
		hPen = CreatePen(PS_SOLID, 3, RGB(128, 0, 0));
		//bRet = TRUE;
		break; 
	case WM_COMMAND:
		wmId    = LOWORD(wParam);
		wmEvent = HIWORD(wParam);
		// Choice in menu:
		switch (wmId)
		{
		case ID_BRUSH_COLOR:

			ZeroMemory(&colorDlg, sizeof(colorDlg));
			colorDlg.lStructSize = sizeof(CHOOSECOLOR); 
			colorDlg.hwndOwner = hWnd;
			colorDlg.lpCustColors = (LPDWORD)colors;  //A pointer to an array of 16 values that contain red, green, blue (RGB) values
			colorDlg.rgbResult = rgbCurrent; // specifies the color initially selected when the dialog box is created
			colorDlg.Flags = CC_RGBINIT;

			if (ChooseColor(&colorDlg))
			{
				DeleteBrush(hBrush);
				hBrush = CreateSolidBrush(colorDlg.rgbResult);
			}
			break;
		case ID_PEN_COLOR:

			ZeroMemory(&colorDlg, sizeof(colorDlg));
			colorDlg.lStructSize = sizeof(CHOOSECOLOR);
			colorDlg.hwndOwner = hWnd;
			colorDlg.lpCustColors = (LPDWORD)colors;
			colorDlg.rgbResult = rgbCurrent;
			colorDlg.Flags = CC_RGBINIT;

			if (ChooseColor(&colorDlg))
			{
				rgbCurrent = colorDlg.rgbResult;
				DeletePen(hPen);
				hPen = CreatePen(PS_SOLID, penWidth, rgbCurrent);
			}
			break;
		case IDM_ABOUT:
			DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
			break;
		case IDM_EXIT:
			DestroyWindow(hWnd);
			break;
		default:
			return DefWindowProc(hWnd, message, wParam, lParam);
		}
		break;
	//Handles pushing left mouse button
	case WM_LBUTTONDOWN:
		fDraw = TRUE;
		ptPrevious.x = LOWORD(lParam);
		ptPrevious.y = HIWORD(lParam);
		break;
	case WM_LBUTTONUP:
		if (fDraw)
		{
			hdc = GetDC(hWnd);
			MoveToEx(hdc, ptPrevious.x, ptPrevious.y, NULL); //set the current pen position to the point with  ptPrevious coordinates
			LineTo(hdc, LOWORD(lParam), HIWORD(lParam));
			ReleaseDC(hWnd, hdc);
			fDraw = FALSE;
		}
		break;
	case WM_MOUSEMOVE:
		if (fDraw)
		{
			hdc = GetDC(hWnd);
			SelectBrush(hdc, hBrush);
			SelectPen(hdc, hPen);
			MoveToEx(hdc, ptPrevious.x, ptPrevious.y, NULL);
			LineTo
				(
				hdc,
				ptPrevious.x = LOWORD(lParam),
				ptPrevious.y = HIWORD(lParam)
				);
			ReleaseDC(hWnd, hdc);
		}
		break;
	case ID_SIZE_1: case ID_SIZE_2: case ID_SIZE_3: case ID_SIZE_4:
		penWidth = LOWORD(wParam) - ID_SIZE_1 + 1;
		DeletePen(hPen);
		hPen = CreatePen(PS_SOLID, penWidth, rgbCurrent);
		break;
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}

	//ReleaseDC(hWnd, hdc);
	return 0;
}

// Handler messages "About"
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);
	switch (message)
	{
	case WM_INITDIALOG:
		return (INT_PTR)TRUE;

	case WM_COMMAND:
		if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
		{
			EndDialog(hDlg, LOWORD(wParam));
			return (INT_PTR)TRUE;
		}
		break;
	}
	return (INT_PTR)FALSE;
}
