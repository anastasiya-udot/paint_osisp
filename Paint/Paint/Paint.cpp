#include "stdafx.h"
#include "Paint.h"
#include <windowsx.h>
#include <commdlg.h>
#include <string>

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
void				Draw(HDC hdc, int toolID);

HDC dcMeta;		//descriptor which remembers user's activity 
static RECT rect;  // rectangle for drawing 
HPEN hPen = NULL;
HBRUSH hBrush = NULL;
BOOL fDraw = FALSE;
POINT ptPrevious = { 0 };
CHOOSECOLOR colorDlg; // struct of standart color dialog
COLORREF colors[16]; //palette
static DWORD rgbCurrent;
static int penWidth = 1;
static POINTS start, end;
static POINT *points;
static int pts = 0;
int toolID = ID_PEN;
static std::wstring str(L"");
bool isDrawn = false;
static bool isFirst = true;




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

HDC InitializeTempDC(HWND hWnd, HDC hdc)
{
	int iWidthMM, iWidthRes, iHeightMM, iHeightRes;

	iWidthMM = GetDeviceCaps(hdc, HORZSIZE);
	iHeightMM = GetDeviceCaps(hdc, VERTSIZE);
	iWidthRes = GetDeviceCaps(hdc, HORZRES);
	iHeightRes = GetDeviceCaps(hdc, VERTRES);

	GetClientRect(hWnd, &rect);

	rect.bottom = (rect.bottom * iHeightMM * 100) / iHeightRes;
	rect.top = (rect.top * iHeightMM * 100) / iHeightRes;
	rect.left = (rect.left * iWidthMM * 100) / iWidthRes;
	rect.right = (rect.right * iWidthMM * 100) / iWidthRes;

	return CreateEnhMetaFile(hdc, (LPWSTR)NULL, &rect, (LPWSTR)NULL);

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
	HDC hdc = GetDC(hWnd); // getting handle device context
	

	switch (message)
	{
	case WM_CREATE:
		dcMeta = InitializeTempDC(hWnd, hdc);
		break;
	case WM_INITDIALOG:
		hPen = CreatePen(PS_SOLID, 3, RGB(128, 0, 0));
		break; 
	case WM_COMMAND:
		wmId    = LOWORD(wParam);
		wmEvent = HIWORD(wParam);
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
		case ID_SIZE_1: case ID_SIZE_2: case ID_SIZE_3: case ID_SIZE_4:
			penWidth = LOWORD(wParam) - ID_SIZE_1 + 1;
			DeletePen(hPen);
			hPen = CreatePen(PS_SOLID, penWidth, rgbCurrent);
			break;
		case IDM_ABOUT:
			DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
			break;
		case IDM_EXIT:
			DestroyWindow(hWnd);
			break;
		default:
			toolID = LOWORD(wParam);
			return DefWindowProc(hWnd, message, wParam, lParam);
		}
		break;
	//Handles pushing left mouse button
	case WM_LBUTTONDOWN:
		if (toolID != ID_POLYGONE)
		{
			start = MAKEPOINTS(lParam);
			isDrawn = false;
		}
		break;
	case WM_LBUTTONUP:
		end = MAKEPOINTS(lParam);

		switch (toolID)
		{
		case ID_ELLIPSE:
		case ID_RECTANGLE:
		case ID_LINE:
			Draw(hdc, toolID);
			Draw(dcMeta, toolID);
			isDrawn = false;
			break;
		case ID_TEXT:
			start = end;
			str.clear();
			break;
		case ID_POLYGONE:
			if (isFirst)
			{
				pts = 0;
				points = (POINT*)calloc(50, sizeof(POINT));
				POINTSTOPOINT(points[pts], end);
				isFirst = false;
			}
			else
			{
				pts++;
				POINTSTOPOINT(points[pts], end);
			}
			Draw(hdc, toolID);
			Draw(dcMeta, toolID);
			break;
		}
		break;
	case WM_MOUSEMOVE:

		if (wParam & MK_LBUTTON)
		{
			if (toolID == ID_PEN)
			{
				end = MAKEPOINTS(lParam);
				Draw(hdc, toolID);
				Draw(dcMeta, toolID);
				start = end;
			}
			else
			{
				SetROP2(hdc, R2_NOTXORPEN); //// sets DrawMode, which indicates how object's inner color will combine with other's colors
				if (isDrawn)
					Draw(hdc, toolID);
				end = MAKEPOINTS(lParam);
				Draw(hdc, toolID);
				isDrawn = true;
			}
		}
		break;
		/*Handles pressing key Esc and ends drawing polyfigures*/
	case WM_KEYDOWN:

		if (wParam == VK_ESCAPE && !isFirst)
		{
			isFirst = true;
			Draw(hdc, toolID);
			Draw(dcMeta, toolID);
			free(points);
		}
		break;
	case WM_CHAR:
		if (toolID == ID_TEXT)
		{
			str.append((wchar_t*)(&wParam));
			TextOut(hdc, start.x, start.y, (LPCWSTR)(&str[0]), str.length());
			TextOut(dcMeta, start.x, start.y, (LPCWSTR)(&str[0]), str.length());
		}
		break;
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}

	ReleaseDC(hWnd, hdc);
	return 0;
}

void Draw(HDC hdc, int toolID){
	SelectBrush(hdc, hBrush);
	SelectPen(hdc, hPen);
	switch (toolID)
	{
	case ID_PEN:
	case ID_LINE:
		MoveToEx(hdc, start.x, start.y, NULL);
		LineTo(hdc, end.x, end.y);
		break;
	case ID_POLYGONE:
		Polyline(hdc, points, pts + 1);
		break;
	case ID_RECTANGLE:
		Rectangle(hdc, start.x, start.y, end.x, end.y);
		break;
	case ID_ELLIPSE:
		Ellipse(hdc, start.x, start.y, end.x, end.y);
		break;
	}
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
