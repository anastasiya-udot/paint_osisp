#include "stdafx.h"
#include "Paint.h"
#include <windowsx.h>
#include <commdlg.h>
#include <string>
#include <winspool.h>
#include <Shellapi.h>


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
void				OnDropFiles(HWND hWnd, HDROP hDrop, HDC hdc);
void				OpenUserFile(HWND hWnd, HENHMETAFILE hemf, OPENFILENAME fileName, HDC hdc);

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
	
	OPENFILENAME fileName;
	TCHAR szFile[MAX_PATH], *szFilter, *szFileTitle, *szTitle;
	HENHMETAFILE hemf, hemfc;
	BOOL dbg;
	PRINTDLG printDlg;
	int wheelDelta = 0;

	switch (message)
	{
	case WM_CREATE:
		dcMeta = InitializeTempDC(hWnd, hdc);
		DragAcceptFiles(hWnd, true);
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
		case ID_NEW:
			GetClientRect(hWnd, &rect);
			FillRect(hdc, &rect, (HBRUSH)(COLOR_WINDOW + 1));
			break;
		case ID_SAVE:

			szTitle = _T("Save as");
			szFile[0] = '\0';
			szFilter = _T("EMF Files\0*.EMF\0\0");

			ZeroMemory(&fileName, sizeof(fileName));
			fileName.lStructSize = sizeof(OPENFILENAME);
			fileName.hwndOwner = hWnd;
			fileName.lpstrFilter = szFilter;
			fileName.lpstrFile = szFile;
			fileName.nMaxFile = sizeof(szFile);
			fileName.lpstrFileTitle = (LPWSTR)NULL;
			fileName.lpstrInitialDir = (LPWSTR)NULL;
			fileName.Flags = OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT;
			fileName.lpstrTitle = szTitle;
			//Creates a Save dialog box that lets the user specify the drive, directory, and name of a file to save.
			if (GetSaveFileName(&fileName) == TRUE)
			{
				//closes an enhanced-metafile device context and returns a handle that identifies an enhanced-format metafile.
				hemf = CloseEnhMetaFile(dcMeta);
				//copies the contents of an enhanced-format metafile to a specified file
				hemfc = CopyEnhMetaFile(hemf, fileName.lpstrFile);
				dbg = DeleteEnhMetaFile(hemf);
				dcMeta = InitializeTempDC(hWnd, hdc);
				dbg = GetClientRect(hWnd, &rect);
				dbg = PlayEnhMetaFile(dcMeta, hemfc, &rect);
				dbg = DeleteEnhMetaFile(hemfc);
			}
			break;
		case ID_OPEN:
			szTitle = _T("Open File");
			szFile[0] = '\0';
			szFilter = _T("EMF Files\0*.EMF\0\0");

			ZeroMemory(&fileName, sizeof(fileName));
			fileName.lStructSize = sizeof(OPENFILENAME);
			fileName.hwndOwner = hWnd;
			fileName.lpstrFilter = szFilter;
			fileName.lpstrFile = szFile;
			fileName.nMaxFile = sizeof(szFile);
			fileName.lpstrFileTitle = (LPWSTR)NULL;
			fileName.lpstrInitialDir = (LPWSTR)NULL;
			fileName.lpstrTitle = szTitle;
			fileName.nFilterIndex = 1;
			fileName.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

			if (GetOpenFileName(&fileName) == TRUE)
			{
				hemf = GetEnhMetaFile(fileName.lpstrFile);
				OpenUserFile(hWnd, hemf, fileName, hdc);
			}
			break;
		case ID_PRINT:
			ZeroMemory(&printDlg, sizeof(printDlg));
			printDlg.lStructSize = sizeof(PRINTDLG);
			printDlg.hwndOwner = hWnd;
			printDlg.hDevMode = NULL;
			printDlg.hDevNames = NULL;
			printDlg.Flags = PD_USEDEVMODECOPIESANDCOLLATE | PD_RETURNDC;
			printDlg.nCopies = 1;
			printDlg.nFromPage = 0xFFFF;
			printDlg.nToPage = 0xFFFF;
			printDlg.nMaxPage = 0XFFFF;
			printDlg.nMinPage = 1;

			if (PrintDlg(&printDlg) == TRUE)
			{
				DOCINFO info;
				HANDLE handle;
				DEVMODE *devMode = (DEVMODE*)GlobalLock(printDlg.hDevMode);

				OpenPrinter(devMode->dmDeviceName, &handle, NULL);
				ZeroMemory(&info, sizeof(DOCINFO));
				info.cbSize = sizeof(DOCINFO);
				info.lpszDatatype = _T("emf");
				info.lpszDocName = _T("NewDoc.emf");
				info.lpszOutput = _T("NewDoc.pdf");

				StartDoc(printDlg.hDC, &info);
				StartPage(printDlg.hDC);

				hemf = CloseEnhMetaFile(dcMeta);
				GetClientRect(hWnd, &rect);
				PlayEnhMetaFile(printDlg.hDC, hemf, &rect);

				EndPage(printDlg.hDC);
				EndDoc(printDlg.hDC);

				ClosePrinter(handle);

				DeleteDC(printDlg.hDC);
			}
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
	case WM_DROPFILES:
		{
			HDROP hDrop = (HDROP)wParam;
			OnDropFiles(hWnd, hDrop, hdc);
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

void OnDropFiles(HWND hWnd,HDROP hDrop, HDC hdc)
{
	TCHAR szFileName[MAX_PATH];
	TCHAR  *szFilter;
	// function to retrieve a count of the files that were dropped and their names
	DWORD dwCount = DragQueryFile(hDrop, 0xFFFFFFFF, szFileName, MAX_PATH);
	for (int i = 0; i < dwCount; i++)
	{
		DragQueryFile(hDrop, i, szFileName, MAX_PATH);
		szFilter = _T("EMF Files\0*.EMF\0\0");
		OPENFILENAME fileName;
		ZeroMemory(&fileName, sizeof(fileName));
		fileName.lStructSize = sizeof(OPENFILENAME);
		fileName.hwndOwner = hWnd;
		fileName.lpstrFilter = szFilter;
		fileName.lpstrFile = (LPWSTR)&szFileName;
		fileName.nMaxFile = sizeof((LPWSTR)&szFileName);
		fileName.lpstrFileTitle = (LPWSTR)NULL;
		fileName.lpstrInitialDir = (LPWSTR)NULL;
		fileName.lpstrTitle = szTitle;
		fileName.nFilterIndex = 1;
		fileName.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

		HENHMETAFILE hemf = GetEnhMetaFile(fileName.lpstrFile);
		OpenUserFile(hWnd, hemf, fileName, hdc);
	}
	DragFinish(hDrop);

	//szFile[0] = '\0';
}

void OpenUserFile(HWND hWnd, HENHMETAFILE hemf, OPENFILENAME fileName, HDC hdc){
	BOOL dbg;
	GetClientRect(hWnd, &rect);
	FillRect(hdc, &rect, SelectBrush(hdc, WHITE_BRUSH));
	FillRect(dcMeta, &rect, SelectBrush(dcMeta, WHITE_BRUSH));
	dbg = PlayEnhMetaFile(hdc, hemf, &rect);
	DeleteEnhMetaFile(hemf);

	hemf = GetEnhMetaFile(fileName.lpstrFile);
	GetClientRect(hWnd, &rect);
	dbg = PlayEnhMetaFile(dcMeta, hemf, &rect);
	DeleteEnhMetaFile(hemf);
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
