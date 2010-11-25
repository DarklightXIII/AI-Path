//-----------------------------------------------------------------
// Game Engine Object
// C++ Source - GameEngine.cpp - version 2010 v2_07
// Copyright Kevin Hoefman - kevin.hoefman@howest.be
// Thanks to Bart Uyttenhove - bart.uyttenhove@howest.be
//		for several changes & contributions
// http://www.digitalartsandentertainment.be/
//-----------------------------------------------------------------

//-----------------------------------------------------------------
// Include Files
//-----------------------------------------------------------------
#include "GameEngine.h"

#define _USE_MATH_DEFINES	// necessary for including (among other values) PI  - see math.h
#include <math.h>			// used in various draw methods
#include <stdio.h>
#include <tchar.h>			// used for unicode strings

#include <iostream>			// iostream, fstream and memory.h used for targa loader
#include <fstream>
#include <memory.h>

//-----------------------------------------------------------------
// Static Variable Initialization
//-----------------------------------------------------------------
GameEngine* GameEngine::m_GameEnginePtr = NULL;

//-----------------------------------------------------------------
// Windows Functions
//-----------------------------------------------------------------
LRESULT CALLBACK WndProc(HWND hWindow, UINT msg, WPARAM wParam, LPARAM lParam)
{
	// Route all Windows messages to the game engine
	return GameEngine::GetSingleton()->HandleEvent(hWindow, msg, wParam, lParam);
}

DWORD WINAPI KeybThreadProc(GameEngine* gamePtr)
{
	return gamePtr->KeybThreadProc();
}

//-----------------------------------------------------------------
// GameEngine Constructor(s)/Destructor
//-----------------------------------------------------------------
GameEngine::GameEngine() :	m_hInstance(NULL), 
							m_hWindow(NULL),
							m_TitlePtr(0),
							m_iFrameDelay(50),		// 20 FPS default
							m_bSleep(true),	
							m_bRunGameLoop(false),
							m_bKeybRunning(true),	// create the keyboard monitoring thread
							m_KeyListPtr(0),
							m_KeybMonitor(0x0),		// binary ; 0 = key not pressed, 1 = key pressed
							m_IsPainting(false),
							m_IsDoublebuffering(false),
							m_ColDraw(RGB(0,0,0)),
							m_FontDraw(0),
							m_GamePtr(0),
							m_PaintDoublebuffered(false),
							m_Fullscreen(false),
							m_WindowRegionPtr(0)
{
	m_hKeybThread = CreateThread(NULL, NULL, (LPTHREAD_START_ROUTINE) ::KeybThreadProc, this, NULL, &m_dKeybThreadID);
}

GameEngine::~GameEngine()
{
	// Clean up the keyboard monitoring thread
	m_bKeybRunning = false;
	WaitForSingleObject( m_hKeybThread, INFINITE );
	CloseHandle( m_hKeybThread );	
	
	// clean up keyboard monitor buffer after the thread that uses it is closed
	if (m_KeyListPtr != 0) 
	{
		delete m_KeyListPtr;
		m_KeyListPtr = 0;
	}

	// clean up the font
	if (m_FontDraw != 0) 
	{
		DeleteObject(m_FontDraw);
		m_FontDraw = 0;
	}

	// clean up title string
	delete m_TitlePtr;
	// clean up the AbstractGame object
	delete m_GamePtr;
}

//-----------------------------------------------------------------
// Game Engine Static Methods
//-----------------------------------------------------------------

GameEngine* GameEngine::GetSingleton()
{
	if ( m_GameEnginePtr == NULL) m_GameEnginePtr = new GameEngine();
	return m_GameEnginePtr;
}

void GameEngine::SetGame(AbstractGame* gamePtr)
{
	m_GamePtr = gamePtr;
}

DWORD GameEngine::KeybThreadProc()
{
	while (m_bKeybRunning)
	{
		if (m_KeyListPtr != NULL)
		{
			int count = 0;
			int key = m_KeyListPtr[0];

			while (key != '\0' && count < (8 * sizeof(unsigned int)))
			{	
				if ( !(GetAsyncKeyState(key)<0) ) // key is not pressed
				{	    
					if (m_KeybMonitor & (0x1 << count)) {
						m_GamePtr->KeyPressed(key); // als de bit op 1 stond, dan firet dit een keypress
					}
					m_KeybMonitor &= ~(0x1 << count);   // de bit wordt op 0 gezet: key is not pressed
				}
				else m_KeybMonitor |= (0x1 << count);	// de bit wordt op 1 gezet: key is pressed

				key = m_KeyListPtr[++count]; // increase count and get next key
			}
		}	

		Sleep(1000 / KEYBCHECKRATE);
	}
	return 0;
}

//-----------------------------------------------------------------
// Game Engine General Methods
//-----------------------------------------------------------------

void GameEngine::SetTitle(String const& strTitleRef)
{
	delete m_TitlePtr; // delete the title string if it already exists
	m_TitlePtr = new String(strTitleRef);
}

bool GameEngine::Run(HINSTANCE hInstance, int iCmdShow)
{
	// create the game engine object, exit if failure
	if (GameEngine::GetSingleton() == NULL) return false;

	// set the instance member variable of the game engine
	GameEngine::GetSingleton()->SetInstance(hInstance);
	 
	// Seed the random number generator
	srand(GetTickCount());

	// Game Initialization
	m_GamePtr->GameInitialize(hInstance);

	// Initialize the game engine
	if (!GameEngine::GetSingleton()->ClassRegister(iCmdShow)) return false;

	// Attach the keyboard thread to the main thread. This gives the keyboard events access to the window state
	// In plain English: this allows a KeyPressed() event to hide the cursor of the window. 
	AttachThreadInput(m_dKeybThreadID, GetCurrentThreadId(), true);

	// Enter the main message loop
	MSG msg;
	DWORD dwTimeTrigger = 0;
	while (true)
	{
		if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			// Process the message
			if (msg.message == WM_QUIT) break;
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		else
		{
			// Make sure the game engine isn't sleeping
			if (!GameEngine::GetSingleton()->GetSleep() && m_bRunGameLoop)
			{
				// Check the time to see if a game cycle has elapsed
				DWORD dwTimeNow = timeGetTime();
				if (dwTimeNow > dwTimeTrigger)
				{
					dwTimeTrigger = dwTimeNow + GameEngine::GetSingleton()->GetFrameDelay();

					// Do user defined keyboard check
					m_GamePtr->CheckKeyboard();

					// Get window, rectangle and HDC
					HWND hWindow = GetWindow();
					RECT WindowClientRect;
					HDC hDC = GetDC(hWindow);
					GetClientRect(hWindow, &WindowClientRect);

					// Double buffering code
					HDC hBufferDC = CreateCompatibleDC(hDC);
					// Create the buffer, size is iWidth and iHeight, NOT the window client rect
					HBITMAP hBufferBmp = CreateCompatibleBitmap(hDC, GetWidth(), GetHeight());
					HBITMAP hOldBmp = (HBITMAP) SelectObject(hBufferDC, hBufferBmp);

					// Do user defined drawing functions on the buffer, parameters added
					// for ease of drawing
					//UPDATE BART: 
					RECT UsedClientRect={0,0,GetWidth(),GetHeight()};
					m_HdcDraw = hBufferDC;
					m_RectDraw = UsedClientRect;
					m_IsDoublebuffering = true;
					m_GamePtr->GameCycle(UsedClientRect);
					m_IsDoublebuffering = false;

					//BitBlt(hDC, 0, 0, iWidth, iHeight, hBufferDC, 0, 0, SRCCOPY);

					// As a last step copy the memdc to the hdc
					//use StretchBlt to scale from m_iWidth and m_iHeight to current window area
					StretchBlt(
							hDC, 0,0,WindowClientRect.right-WindowClientRect.left,WindowClientRect.bottom-WindowClientRect.top,
							hBufferDC, 0, 0, GetWidth(),GetHeight(),SRCCOPY
					);

					// Reset the old bmp of the buffer, mainly for show since we kill it anyway
					SelectObject(hBufferDC, hOldBmp);
					// Kill the buffer
					DeleteObject(hBufferBmp);
					DeleteDC(hBufferDC);

					// Release HDC
					ReleaseDC(hWindow, hDC);
				}                             
				else Sleep(1);//Sleep for one ms te bring cpu load from 100% to 1%. if removed this loops like roadrunner
			}
			else WaitMessage(); // if the engine is sleeping or the game loop isn't supposed to run, wait for the next windows message.
		}
	}
	return msg.wParam?true:false;
}

bool GameEngine::SetGameValues(String const& TitleRef, WORD wIcon, WORD wSmallIcon, int iWidth = 640, int iHeight = 480)
{
	SetTitle(TitleRef);
	SetIcon(wIcon);
	SetSmallIcon(wSmallIcon);
	SetWidth(iWidth);
	SetHeight(iHeight);

	return true;
}

void GameEngine::ShowMousePointer(bool value)
{
	// set the value
	ShowCursor(value);	
	
	// redraw the screen
	InvalidateRect(m_hWindow, 0, true);
}

bool GameEngine::SetWindowRegion(HitRegion* regionPtr)
{
	if (m_Fullscreen) return false;

	if (regionPtr == 0) 
	{	
		/*
		// switch on the title bar
	    DWORD dwStyle = GetWindowLong(m_hWindow, GWL_STYLE);
	    dwStyle |= WS_CAPTION;
	    SetWindowLong(m_hWindow, GWL_STYLE, dwStyle);
		*/

		// turn off window region
		SetWindowRgn(m_hWindow, 0, true);

		// delete the buffered window region (if it exists)
		delete m_WindowRegionPtr;
		m_WindowRegionPtr = 0;
	}
	else 
	{
		// if there is already a window region set, release the buffered region object
		if (m_WindowRegionPtr != 0)
		{
			// turn off window region for safety
			SetWindowRgn(m_hWindow, 0, true);
				
			// delete the buffered window region 
			delete m_WindowRegionPtr;
			m_WindowRegionPtr = 0;
		}

		/*
		// switch off the title bar
	    DWORD dwStyle = GetWindowLong(m_hWindow, GWL_STYLE);
	    dwStyle &= ~WS_CAPTION;
	    SetWindowLong(m_hWindow, GWL_STYLE, dwStyle);
		*/

		// create a copy of the submitted region (windows will lock the region handle that it receives)
		m_WindowRegionPtr = regionPtr->Clone();

		// translate region coordinates in the client field to window coordinates, taking title bar and frame into account
		m_WindowRegionPtr->Move(GetSystemMetrics(SM_CXFIXEDFRAME), GetSystemMetrics(SM_CYFIXEDFRAME) + GetSystemMetrics(SM_CYCAPTION));

		// set the window region
		SetWindowRgn(m_hWindow, m_WindowRegionPtr->GetHandle(), true);
	}

	return true;
}

bool GameEngine::HasWindowRegion()
{
	return (m_WindowRegionPtr?true:false);
}

bool GameEngine::GoFullscreen()
{
	// exit if already in fullscreen mode
	if (m_Fullscreen) return false;

	// turn off window region without redraw
	SetWindowRgn(m_hWindow, 0, false);

	DEVMODE newSettings;	

	// request current screen settings
	EnumDisplaySettings(0, 0, &newSettings);

	//  set desired screen size/res	
 	newSettings.dmPelsWidth  = GetWidth();		
	newSettings.dmPelsHeight = GetHeight();		
	newSettings.dmBitsPerPel = 32;		

	//specify which aspects of the screen settings we wish to change 
 	newSettings.dmFields = DM_BITSPERPEL | DM_PELSWIDTH | DM_PELSHEIGHT;

	// attempt to apply the new settings 
	long result = ChangeDisplaySettings(&newSettings, CDS_FULLSCREEN);

	// exit if failure, else set datamember to fullscreen and return true
	if ( result != DISP_CHANGE_SUCCESSFUL )	return false;
	else 
	{
		// store the location of the window
		m_OldLoc = GetLocation();

		// switch off the title bar
	    DWORD dwStyle = GetWindowLong(m_hWindow, GWL_STYLE);
	    dwStyle &= ~WS_CAPTION;
	    SetWindowLong(m_hWindow, GWL_STYLE, dwStyle);

		// move the window to (0,0)
		SetWindowPos(m_hWindow, 0, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
		InvalidateRect(m_hWindow, 0, true);		

		m_Fullscreen = true;

		return true;
	}
}

bool GameEngine::GoWindowedMode()
{
	// exit if already in windowed mode
	if (!m_Fullscreen) return false;

	// this resets the screen to the registry-stored values
  	ChangeDisplaySettings(0, 0);

	// replace the title bar
	DWORD dwStyle = GetWindowLong(m_hWindow, GWL_STYLE);
    dwStyle = dwStyle | WS_CAPTION;
    SetWindowLong(m_hWindow, GWL_STYLE, dwStyle);

	// move the window back to its old position
	SetWindowPos(m_hWindow, 0, m_OldLoc.x, m_OldLoc.y, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
	InvalidateRect(m_hWindow, 0, true);

	m_Fullscreen = false;

	return true;
}

bool GameEngine::IsFullscreen()
{
	return m_Fullscreen;
}

bool GameEngine::ClassRegister(int iCmdShow)
{
  WNDCLASSEX    wndclass;

  // Create the window class for the main window
  wndclass.cbSize         = sizeof(wndclass);
  wndclass.style          = CS_HREDRAW | CS_VREDRAW;
  wndclass.lpfnWndProc    = WndProc;
  wndclass.cbClsExtra     = 0;
  wndclass.cbWndExtra     = 0;
  wndclass.hInstance      = m_hInstance;
  wndclass.hIcon          = LoadIcon(m_hInstance, MAKEINTRESOURCE(GetIcon()));
  wndclass.hIconSm        = LoadIcon(m_hInstance, MAKEINTRESOURCE(GetSmallIcon()));
  wndclass.hCursor        = LoadCursor(NULL, IDC_ARROW);
  wndclass.hbrBackground  = m_PaintDoublebuffered?NULL:(HBRUSH)(COLOR_WINDOW + 1);
  wndclass.lpszMenuName   = NULL;
  wndclass.lpszClassName  = m_TitlePtr->ToTChar();

  // Register the window class
  if (!RegisterClassEx(&wndclass))
    return false;

  // Calculate the window size and position based upon the game size
  int iWindowWidth = m_iWidth + GetSystemMetrics(SM_CXFIXEDFRAME) * 2,
      iWindowHeight = m_iHeight + GetSystemMetrics(SM_CYFIXEDFRAME) * 2 +
        GetSystemMetrics(SM_CYCAPTION);
  if (wndclass.lpszMenuName != NULL)
    iWindowHeight += GetSystemMetrics(SM_CYMENU);
  int iXWindowPos = (GetSystemMetrics(SM_CXSCREEN) - iWindowWidth) / 2,
      iYWindowPos = (GetSystemMetrics(SM_CYSCREEN) - iWindowHeight) / 2;

  // Create the window
  m_hWindow = CreateWindow(m_TitlePtr->ToTChar(), m_TitlePtr->ToTChar(), 
		WS_POPUPWINDOW | WS_CAPTION | WS_MINIMIZEBOX/*uncomment this to enable scaling when programming a game| WS_MAXIMIZEBOX*/ | WS_CLIPCHILDREN, 
		iXWindowPos, iYWindowPos, iWindowWidth,
		iWindowHeight, NULL, NULL, m_hInstance, NULL);
  if (!m_hWindow)
    return false;

  // Show and update the window
  ShowWindow(m_hWindow, iCmdShow);
  UpdateWindow(m_hWindow);

  return true;
}

bool GameEngine::IsKeyDown(int vKey)
{
	if (GetAsyncKeyState(vKey) < 0) return true;
	else return false;
}

void GameEngine::SetKeyList(String const& keyListRef)
{
	if (keyListRef.ToTChar() != NULL) delete m_KeyListPtr; // clear lijst als er al een lijst bestaat

	int iLength = 0;

	while (keyListRef.ToTChar()[iLength] != '\0') iLength++; // tellen hoeveel tekens er zijn

	m_KeyListPtr = (TCHAR*) malloc((iLength + 1) * sizeof(TCHAR)); // maak plaats voor zoveel tekens + 1 

	for (int count = 0; count < iLength + 1; count++) 
	{
		TCHAR key = keyListRef.ToTChar()[count]; 
		m_KeyListPtr[count] = (key > 96 && key < 123)? key-32 : key; // vul het teken in, in uppercase indien kleine letter

	}
}

void GameEngine::QuitGame()
{
	PostMessage(GameEngine::GetWindow(), WM_DESTROY, 0, 0);
}

void GameEngine::MessageBox(String const& textRef)
{
	if (sizeof(TCHAR) == 2)	MessageBoxW(GetWindow(), (wchar_t*) textRef.ToTChar(), (wchar_t*) m_TitlePtr->ToTChar(), MB_ICONEXCLAMATION | MB_OK);
	else MessageBoxA(GetWindow(), (char*) textRef.ToTChar(), (char*) m_TitlePtr->ToTChar(), MB_ICONEXCLAMATION | MB_OK);
}

void GameEngine::MessageBox(int value)
{
	MessageBox(String("") + value);
}

void GameEngine::MessageBox(size_t value)
{
	MessageBox(String("") + value);
}

void GameEngine::MessageBox(double value)
{
	MessageBox(String("") + value);
}

static bool CALLBACK EnumInsertChildrenProc(HWND hwnd, LPARAM lParam)
{
	vector<HWND> *row = (vector<HWND> *) lParam; 

	row->push_back(hwnd); // elk element invullen in de vector

	return true;
}

void GameEngine::TabNext(HWND ChildWindow)
{
	vector<HWND> childWindows; 

	EnumChildWindows(m_hWindow, (WNDENUMPROC) EnumInsertChildrenProc, (LPARAM) &childWindows);

	int position = 0;
	HWND temp = childWindows[position];
	while(temp != ChildWindow) temp = childWindows[++position]; // positie van childWindow in de vector opzoeken 

	if (position == childWindows.size() - 1) SetFocus(childWindows[0]);
	else SetFocus(childWindows[position + 1]);
}

void GameEngine::TabPrevious(HWND ChildWindow)
{	
	vector<HWND> childWindows;

	EnumChildWindows(m_hWindow, (WNDENUMPROC) EnumInsertChildrenProc, (LPARAM) &childWindows);

	int position = (int) childWindows.size() - 1;
	HWND temp = childWindows[position];
	while(temp != ChildWindow) temp = childWindows[--position]; // positie van childWindow in de vector opzoeken 

	if (position == 0) SetFocus(childWindows[childWindows.size() - 1]);
	else SetFocus(childWindows[position - 1]);
}

bool GameEngine::DrawLine(int x1, int y1, int x2, int y2)
{
	HPEN hOldPen, hNewPen = CreatePen(PS_SOLID, 1, m_ColDraw);
	hOldPen = (HPEN) SelectObject(m_HdcDraw, hNewPen);
	MoveToEx(m_HdcDraw, x1, y1, NULL);
	LineTo(m_HdcDraw, x2, y2);
	MoveToEx(m_HdcDraw, 0, 0, NULL); // reset van de positie - zorgt ervoor dat bvb AngleArc vanaf 0,0 gaat tekenen ipv de laatste positie van de DrawLine
	SelectObject(m_HdcDraw, hOldPen);
	DeleteObject(hNewPen);
	
	return true;
}

bool GameEngine::DrawPolygon(const POINT ptsArr[], int count, bool close)
{
	HPEN hOldPen, hNewPen = CreatePen(PS_SOLID, 1, m_ColDraw);
	hOldPen = (HPEN) SelectObject(m_HdcDraw, hNewPen);

	FormPolygon(ptsArr, count, close);

	SelectObject(m_HdcDraw, hOldPen);
	DeleteObject(hNewPen);	

	return true;
}

bool GameEngine::DrawPolygon(const POINT ptsArr[], int count)
{
	if (m_IsDoublebuffering || m_IsPainting) return DrawPolygon(ptsArr, count, false);
	else return false;
}

bool GameEngine::FillPolygon(const POINT ptsArr[], int count, bool close)
{
	HPEN hOldPen, hNewPen = CreatePen(PS_SOLID, 1, m_ColDraw);
	HBRUSH hOldBrush, hNewBrush = CreateSolidBrush(m_ColDraw);
	hOldPen = (HPEN) SelectObject(m_HdcDraw, hNewPen);
	hOldBrush = (HBRUSH) SelectObject(m_HdcDraw, hNewBrush);

	BeginPath(m_HdcDraw);

	FormPolygon(ptsArr, count, close);

	EndPath(m_HdcDraw);
	StrokeAndFillPath(m_HdcDraw);

	SelectObject(m_HdcDraw, hOldPen);
	SelectObject(m_HdcDraw, hOldBrush);

	DeleteObject(hNewPen);
	DeleteObject(hNewBrush);

	return true;
}

bool GameEngine::FillPolygon(const POINT ptsArr[], int count)
{
	if (m_IsDoublebuffering || m_IsPainting) return FillPolygon(ptsArr, count, false);
	else return false;
}

void GameEngine::FormPolygon(const POINT ptsArr[], int count, bool close)
{
	if (!close) Polyline(m_HdcDraw, ptsArr, count);
	else
	{
		POINT* newpts= new POINT[count+1]; // interessant geval: deze code werkt niet met memory allocation at compile time => demo case for dynamic memory use
		for (int i = 0; i < count; i++) newpts[i] = ptsArr[i];
		newpts[count] = ptsArr[0];

		Polyline(m_HdcDraw, newpts, count+1);

		delete[] newpts;
	}
}

bool GameEngine::DrawRect(int x, int y, int width, int height)
{
	HPEN hOldPen, hNewPen = CreatePen(PS_SOLID, 1, m_ColDraw);
	hOldPen = (HPEN) SelectObject(m_HdcDraw, hNewPen);
	
	POINT pts[4] = {x, y, x + width -1, y, x + width-1, y + height-1, x, y + height-1};
	DrawPolygon(pts, 4, true);

	SelectObject(m_HdcDraw, hOldPen);
	DeleteObject(hNewPen);	

	return true;
}

bool GameEngine::FillRect(int x, int y, int width, int height)
{
	HBRUSH hOldBrush, hNewBrush = CreateSolidBrush(m_ColDraw);
	HPEN hOldPen, hNewPen = CreatePen(PS_SOLID, 1, m_ColDraw);

	hOldBrush = (HBRUSH) SelectObject(m_HdcDraw, hNewBrush);
	hOldPen = (HPEN) SelectObject(m_HdcDraw, hNewPen);
	
	Rectangle(m_HdcDraw, x, y, x + width, y + height);
						
	SelectObject(m_HdcDraw, hOldPen);
	SelectObject(m_HdcDraw, hOldBrush);

	DeleteObject(hNewPen);
	DeleteObject(hNewBrush);

	return true;
}

bool GameEngine::DrawRoundRect(int x, int y, int width, int height, int radius)
{
	HPEN hOldPen, hNewPen = CreatePen(PS_SOLID, 1, m_ColDraw);
	hOldPen = (HPEN) SelectObject(m_HdcDraw, hNewPen);
	
	BeginPath(m_HdcDraw);

	RoundRect(m_HdcDraw, x, y, x + width, y + height, radius, radius);

	EndPath(m_HdcDraw);
	StrokePath(m_HdcDraw);

	SelectObject(m_HdcDraw, hOldPen);
	DeleteObject(hNewPen);	

	return true;
}

bool GameEngine::FillRoundRect(int x, int y, int width, int height, int radius)
{
	HBRUSH hOldBrush, hNewBrush = CreateSolidBrush(m_ColDraw);
	HPEN hOldPen, hNewPen = CreatePen(PS_SOLID, 1, m_ColDraw);

	hOldBrush = (HBRUSH) SelectObject(m_HdcDraw, hNewBrush);
	hOldPen = (HPEN) SelectObject(m_HdcDraw, hNewPen);
	
	RoundRect(m_HdcDraw, x, y, x + width, y + height, radius, radius);
						
	SelectObject(m_HdcDraw, hOldPen);
	SelectObject(m_HdcDraw, hOldBrush);

	DeleteObject(hNewPen);
	DeleteObject(hNewBrush);

	return true;
}

bool GameEngine::DrawOval(int x, int y, int width, int height)
{
	HPEN hOldPen, hNewPen = CreatePen(PS_SOLID, 1, m_ColDraw);
	hOldPen = (HPEN) SelectObject(m_HdcDraw, hNewPen);
	
	Arc(m_HdcDraw, x, y, x + width, y + height, x, y + height/2, x, y + height/2);

	SelectObject(m_HdcDraw, hOldPen);
	DeleteObject(hNewPen);	

	return true;
}

bool GameEngine::FillOval(int x, int y, int width, int height)
{
	HBRUSH hOldBrush, hNewBrush = CreateSolidBrush(m_ColDraw);
	HPEN hOldPen, hNewPen = CreatePen(PS_SOLID, 1, m_ColDraw);

	hOldBrush = (HBRUSH) SelectObject(m_HdcDraw, hNewBrush);
	hOldPen = (HPEN) SelectObject(m_HdcDraw, hNewPen);
	
	Ellipse(m_HdcDraw, x, y, x + width, y + height);
						
	SelectObject(m_HdcDraw, hOldPen);
	SelectObject(m_HdcDraw, hOldBrush);

	DeleteObject(hNewPen);
	DeleteObject(hNewBrush);

	return true;
}

bool GameEngine::DrawArc(int x, int y, int width, int height, int startDegree, int angle)
{
	if (angle == 0) return false;
	if (angle > 360) { DrawOval(x, y, width, height); }
	else
	{
		HPEN hOldPen, hNewPen = CreatePen(PS_SOLID, 1, m_ColDraw);
		hOldPen = (HPEN) SelectObject(m_HdcDraw, hNewPen);
		
		POINT ptStart = AngleToPoint(x, y, width, height, startDegree);
		POINT ptEnd = AngleToPoint(x, y, width, height, startDegree + angle);
		
		if (angle > 0) Arc(m_HdcDraw, x, y, x + width, y + height, ptStart.x, ptStart.y, ptEnd.x, ptEnd.y);
		else Arc(m_HdcDraw, x, y, x + width, y + height, ptEnd.x, ptEnd.y, ptStart.x, ptStart.y);

		SelectObject(m_HdcDraw, hOldPen);
		DeleteObject(hNewPen);	
	}

	return true;
}

bool GameEngine::FillArc(int x, int y, int width, int height, int startDegree, int angle)
{	
	if (angle == 0) return false;
	if (angle > 360) { FillOval(x, y, width, height); }
	else
	{
		HBRUSH hOldBrush, hNewBrush = CreateSolidBrush(m_ColDraw);
		HPEN hOldPen, hNewPen = CreatePen(PS_SOLID, 1, m_ColDraw);

		hOldBrush = (HBRUSH) SelectObject(m_HdcDraw, hNewBrush);
		hOldPen = (HPEN) SelectObject(m_HdcDraw, hNewPen);

		POINT ptStart = AngleToPoint(x, y, width, height, startDegree);
		POINT ptEnd = AngleToPoint(x, y, width, height, startDegree + angle);
		
		if (angle >0) Pie(m_HdcDraw, x, y, x + width, y + height, ptStart.x, ptStart.y, ptEnd.x, ptEnd.y);
		else Pie(m_HdcDraw, x, y, x + width, y + height, ptEnd.x, ptEnd.y, ptStart.x, ptStart.y);

		SelectObject(m_HdcDraw, hOldPen);
		SelectObject(m_HdcDraw, hOldBrush);

		DeleteObject(hNewPen);
		DeleteObject(hNewBrush);
	}

	return true;
}

POINT GameEngine::AngleToPoint(int x, int y, int width, int height, int angle)
{
	POINT pt;

	// if necessary adjust angle so that it has a value between 0 and 360 degrees
	if (angle > 360 || angle < -360) angle = angle % 360;
	if (angle < 0) angle += 360;

	// default values for standard angles
	if (angle == 0) { pt.x = x + width; pt.y = y + (int) (height / 2); }
	else if (angle == 90) { pt.x = x + (int) (width / 2); pt.y = y; }
	else if (angle == 180) { pt.x = x; pt.y = y + (int) (height / 2); }
	else if (angle == 270) { pt.x = x + (int) (width / 2); pt.y = y + height; }
	// else calculate non-default values
	else
	{
		// point on the ellipse = "stelsel" of the cartesian equation of the ellipse combined with y = tg(alpha) * x
		// using the equation for ellipse with 0,0 in the center of the ellipse
		double aSquare = pow(width/2.0, 2);
		double bSquare = pow(height/2.0, 2);
		double tangens = tan(angle * M_PI / 180);
		double tanSquare = pow(tangens, 2);

		// calculate x
		pt.x = (long) sqrt( aSquare * bSquare / (bSquare + tanSquare * aSquare));
		if (angle > 90 && angle < 270) pt.x *= -1; // sqrt returns the positive value of the square, take the negative value if necessary

		// calculate y
		pt.y = (long) (tangens * pt.x);
		pt.y *= -1;	// reverse the sign because of inverted y-axis

		// offset the ellipse into the screen
		pt.x += x + (int)(width / 2);
		pt.y += y + (int)(height / 2);
	}

	return pt;
}

int GameEngine::DrawString(String const& textRef, int x, int y, int width, int height)
{
	HFONT hOldFont;
	COLORREF oldColor;

	if (m_FontDraw != 0) hOldFont = (HFONT) SelectObject(m_HdcDraw, m_FontDraw);

	oldColor = SetTextColor(m_HdcDraw, m_ColDraw);
	SetBkMode(m_HdcDraw, TRANSPARENT);
	
	RECT rc = {x, y, x + width - 1, y + height - 1};
	int result = DrawText(m_HdcDraw, textRef.ToTChar(), -1, &rc, DT_WORDBREAK);

	SetBkMode(m_HdcDraw, OPAQUE);
	SetTextColor(m_HdcDraw, oldColor);
	if (m_FontDraw != 0) SelectObject(m_HdcDraw, hOldFont);

	return result;
}

int GameEngine::DrawString(String const& textRef, int x, int y)
{
	HFONT hOldFont;
	COLORREF oldColor;

	if (m_FontDraw != 0) hOldFont = (HFONT) SelectObject(m_HdcDraw, m_FontDraw);
	
	oldColor = SetTextColor(m_HdcDraw, m_ColDraw);
	SetBkMode(m_HdcDraw, TRANSPARENT);

	int count = 0;
	while (textRef.ToTChar()[count] != '\0') count++;

	int result = TextOut(m_HdcDraw, x, y, textRef.ToTChar(), count);

	SetBkMode(m_HdcDraw, OPAQUE);
	SetTextColor(m_HdcDraw, oldColor);

	if (m_FontDraw != 0) SelectObject(m_HdcDraw, hOldFont);

	return result;
}

bool GameEngine::DrawBitmap(Bitmap* bitmapPtr, int x, int y, RECT rect)
{
	if (!bitmapPtr->Exists()) return false;

	int opacity = bitmapPtr->GetOpacity();

	if (opacity == 0 && bitmapPtr->HasAlphaChannel()) return true; // don't draw if opacity == 0 and opacity is used

	HDC hdcMem = CreateCompatibleDC(m_HdcDraw);
	HBITMAP hbmOld = (HBITMAP) SelectObject(hdcMem, bitmapPtr->GetHandle());

	if (bitmapPtr->HasAlphaChannel())
	{
		BLENDFUNCTION blender={AC_SRC_OVER, 0, (int) (2.55 * opacity), AC_SRC_ALPHA}; // blend function combines opacity and pixel based transparency
		AlphaBlend(m_HdcDraw, x, y, rect.right - rect.left, rect.bottom - rect.top, hdcMem, rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top, blender);
	}
	else TransparentBlt(m_HdcDraw, x, y, rect.right - rect.left, rect.bottom - rect.top, hdcMem, rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top, bitmapPtr->GetTransparencyColor());

	SelectObject(hdcMem, hbmOld);
	DeleteDC(hdcMem);

	return true;
}

bool GameEngine::DrawBitmap(Bitmap* bitmapPtr, int x, int y)
{
	if (!bitmapPtr->Exists()) return false;

    BITMAP bm;
    GetObject(bitmapPtr->GetHandle(), sizeof(bm), &bm);
	RECT rect = {0, 0, bm.bmWidth, bm.bmHeight};

    return DrawBitmap(bitmapPtr, x, y, rect);
}

bool GameEngine::DrawSolidBackground(COLORREF color)
{
	COLORREF oldColor = GetDrawColor();
	SetColor(color);
	FillRect(0, 0, m_RectDraw.right, m_RectDraw.bottom);
	SetColor(oldColor);

	return true;
}

bool GameEngine::Repaint() 
{
	return InvalidateRect(m_hWindow, NULL, true)?true:false;
}

POINT GameEngine::GetLocation()
{
	RECT info;
	POINT pos;

	GetWindowRect(m_hWindow, &info);
	pos.x = info.left;
	pos.y = info.top;

	return pos;
}

void GameEngine::SetLocation(int x, int y)
{
	SetWindowPos(m_hWindow, 0, x, y, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
	InvalidateRect(m_hWindow, 0, TRUE);
}

void GameEngine::SetFont(String const& fontNameRef, bool bold, bool italic, bool underline, int size)
{
	if (m_FontDraw != 0) DeleteObject(m_FontDraw);

	LOGFONT ft;
	ZeroMemory(&ft, sizeof(ft));

	_tcscpy_s(ft.lfFaceName, sizeof(ft.lfFaceName) / sizeof(TCHAR), fontNameRef.ToTChar());
	ft.lfStrikeOut = 0;
	ft.lfUnderline = underline?1:0;
	ft.lfHeight = size;
    ft.lfEscapement = 0;
	ft.lfWeight = bold?FW_BOLD:0;
	ft.lfItalic = italic?1:0;

    m_FontDraw = CreateFontIndirect(&ft);
}

void GameEngine::RunGameLoop(bool value)
{
	m_bRunGameLoop = value;
}

LRESULT GameEngine::HandleEvent(HWND hWindow, UINT msg, WPARAM wParam, LPARAM lParam)
{
	HDC         hDC;
	PAINTSTRUCT ps;

	// Get window rectangle and HDC
	RECT WindowClientRect;
	GetClientRect(hWindow, &WindowClientRect);
	RECT UsedClientRect;
	UsedClientRect.left=UsedClientRect.top=0;
	UsedClientRect.right = GetWidth();
	UsedClientRect.bottom = GetHeight();

	double ScaleX=1,ScaleY=1;
	if(WindowClientRect.right!=0 &&WindowClientRect.bottom!=0)
	{
		ScaleX = (UsedClientRect.right-UsedClientRect.left)/(double)(WindowClientRect.right-WindowClientRect.left);
		ScaleY = (UsedClientRect.bottom-UsedClientRect.top)/(double)(WindowClientRect.bottom-WindowClientRect.top);
	}
	// Double buffering code
	HDC hBufferDC;
	HBITMAP hBufferBmp;
	HBITMAP hOldBmp;


	// Route Windows messages to game engine member functions
	switch (msg)
	{
		case WM_CREATE:
			// Set the game window and start the game
			SetWindow(hWindow);
			// User defined functions for start of the game
			m_GamePtr->GameStart(); 

			return 0;

		case WM_ACTIVATE:
			// Activate/deactivate the game and update the Sleep status
			if (wParam != WA_INACTIVE)
			{
				//Lock hDC
				hDC = GetDC(hWindow);

				// Do user defined drawing functions
				//m_GamePtr->GameActivate(hDC, UsedClientRect);
				m_GamePtr->GameActivate();

				// Release HDC
				ReleaseDC(hWindow, hDC);

				SetSleep(false);
			}
			else
			{			
				//Lock hDC
				hDC = GetDC(hWindow);

				// Do user defined drawing functions
				//m_GamePtr->GameDeactivate(hDC, UsedClientRect);
				m_GamePtr->GameDeactivate();

				// Release HDC
				ReleaseDC(hWindow, hDC);
			}
			return 0;

		case WM_PAINT:
			if (m_PaintDoublebuffered)
			{
				// Get window, rectangle and HDC
				hDC = BeginPaint(hWindow, &ps);

				// Double buffering code
				hBufferDC = CreateCompatibleDC(hDC);
				// Create the buffer, size is area used by client
				hBufferBmp = CreateCompatibleBitmap(hDC, GetWidth(), GetHeight());
				hOldBmp = (HBITMAP) SelectObject(hBufferDC, hBufferBmp);

				// Do user defined drawing functions on the buffer, parameters added
				// for ease of drawing
				m_HdcDraw = hBufferDC;
				m_RectDraw = UsedClientRect;

				m_IsPainting = true;
				m_GamePtr->GamePaint(UsedClientRect);
				m_IsPainting = false;

				// As a last step copy the memdc to the hdc
				//BitBlt(hDC, 0, 0, iWidth, iHeight, hBufferDC, 0, 0, SRCCOPY);
				StretchBlt(
						hDC, 0,0,WindowClientRect.right-WindowClientRect.left,WindowClientRect.bottom-WindowClientRect.top,
						hBufferDC, 0, 0, GetWidth(),GetHeight(),SRCCOPY
				);

				// Reset the old bmp of the buffer, mainly for show since we kill it anyway
				SelectObject(hBufferDC, hOldBmp);
				// Kill the buffer
				DeleteObject(hBufferBmp);
				DeleteDC(hBufferDC);

				// end paint
				EndPaint(hWindow, &ps);
			}
			else
			{
				m_HdcDraw = BeginPaint(hWindow, &ps);	
				GetClientRect(hWindow, &m_RectDraw);

				m_IsPainting = true;
				m_GamePtr->GamePaint(m_RectDraw);
				m_IsPainting = false;

				EndPaint(hWindow, &ps);
			}

			return 0;

		case WM_CTLCOLOREDIT:
			return SendMessage((HWND) lParam, WM_CTLCOLOREDIT, wParam, lParam);	// delegate this message to the child window

		case WM_CTLCOLORBTN:
			return SendMessage((HWND) lParam, WM_CTLCOLOREDIT, wParam, lParam);	// delegate this message to the child window

		case WM_LBUTTONDOWN:
			m_GamePtr->MouseButtonAction(true, true, static_cast<int>(LOWORD(lParam)*ScaleX),static_cast<int>( HIWORD(lParam)*ScaleY), wParam);
			return 0;

		case WM_LBUTTONUP:
			m_GamePtr->MouseButtonAction(true, false, static_cast<int>(LOWORD(lParam)*ScaleX),static_cast<int>( HIWORD(lParam)*ScaleY), wParam);
			return 0;

		case WM_RBUTTONDOWN:
			m_GamePtr->MouseButtonAction(false, true, static_cast<int>(LOWORD(lParam)*ScaleX),static_cast<int>( HIWORD(lParam)*ScaleY), wParam);
			return 0;

		case WM_RBUTTONUP:
			m_GamePtr->MouseButtonAction(false, false, static_cast<int>(LOWORD(lParam)*ScaleX),static_cast<int>( HIWORD(lParam)*ScaleY), wParam);
			return 0;

		case WM_MOUSEMOVE:
			m_GamePtr->MouseMove(static_cast<int>(LOWORD(lParam)*ScaleX),static_cast<int>( HIWORD(lParam)*ScaleY), wParam);
			return 0;

		case WM_SYSCOMMAND:	// trapping this message prevents a freeze after the ALT key is released
			if (wParam == SC_KEYMENU) return 0;			// see win32 API : WM_KEYDOWN
			else break;    

		case WM_DESTROY:
			// User defined code for exiting the game
			m_GamePtr->GameEnd();
			// Delete the game engine
			delete GameEngine::GetSingleton();
			
			// End the game and exit the application
			PostQuitMessage(0);
			return 0;

		case WM_SIZE:
			if(wParam==SIZE_MAXIMIZED)
			{
				// switch off the title bar
				DWORD dwStyle = GetWindowLong(m_hWindow, GWL_STYLE);
				dwStyle &= ~WS_CAPTION;
				SetWindowLong(m_hWindow, GWL_STYLE, dwStyle);
				//If you have changed certain window data using SetWindowLong, you must call SetWindowPos for the changes to take effect.
				SetWindowPos(m_hWindow, 0, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE | SWP_NOZORDER | SWP_FRAMECHANGED);
				return 0;
			}
			return 0;
		//use ESC key to go from fullscreen window to smaller window
		case WM_KEYDOWN:
			switch (wParam)
			{
			case VK_ESCAPE:
				//alleen als fullscreen gewerkt wordt
				RECT WindowRect;
				GetWindowRect(hWindow,&WindowRect);
				if((WindowRect.right-WindowRect.left)>=GetSystemMetrics(SM_CXSCREEN))
				{
					// turns title bar on/off
					DWORD dwStyle = GetWindowLong(m_hWindow, GWL_STYLE);
					if(dwStyle & WS_CAPTION)dwStyle &= ~WS_CAPTION;
					else dwStyle = dwStyle | WS_CAPTION;
					SetWindowLong(m_hWindow, GWL_STYLE, dwStyle);
					//dit zou moeten gecalled worden, maar na de lijn, is getclientrect verkeerd.
					//SetWindowPos(m_hWindow, 0, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE | SWP_NOZORDER | SWP_FRAMECHANGED);
			}
				return 0;
		}
		return 0;

	}
	return DefWindowProc(hWindow, msg, wParam, lParam);
}


//-----------------------------------------------------------------
// Caller methods
//-----------------------------------------------------------------
bool Caller::AddActionListener(Callable* targetPtr)
{
	return AddListenerObject(targetPtr);
}	

bool Caller::RemoveActionListener(Callable* targetPtr) 
{
	return RemoveListenerObject(targetPtr);
}

bool Caller::CallListeners()   
{			
	for (vector<Callable*>::iterator it = m_TargetList.begin(); it != m_TargetList.end(); ++it)
	{
		(*it)->CallAction(this);	
	}

	return (m_TargetList.size() > 0);
}

bool Caller::AddListenerObject(Callable* targetPtr) 
{
	for (vector<Callable*>::iterator it = m_TargetList.begin(); it != m_TargetList.end(); ++it)
	{
		if ((*it) == targetPtr) return false;
	}

	m_TargetList.push_back(targetPtr);
	return true;
}
	
bool Caller::RemoveListenerObject(Callable* targetPtr) 
{
	vector<Callable*>::iterator pos = find(m_TargetList.begin(), m_TargetList.end(), targetPtr); // find algorithm from STL

	if (pos == m_TargetList.end()) return false;
	else
	{
		m_TargetList.erase(pos);
		return true;
	}
}

//-----------------------------------------------------------------
// Bitmap methods
//-----------------------------------------------------------------

// set static datamember to zero
int Bitmap::m_nr = 0;

Bitmap::Bitmap(String const& nameRef, bool createAlphaChannel): m_Handle(0), m_TransparencyKey(-1), m_Opacity(100), m_PixelsPtr(0), m_Exists(false)
{
	// check if the file to load is a targa
	if (nameRef.EndsWith(".tga")) 
	{
		m_IsTarga = true;
		m_HasAlphaChannel = true;
		TargaLoader* targa = new TargaLoader();

		if (targa->Load(nameRef.ToTChar()) == 1)
		{
			m_Handle = CreateBitmap(targa->GetWidth(), targa->GetHeight(), 1, targa->GetBPP(), (void*)targa->GetImg());
			if (m_Handle != 0) m_Exists = true;
		}
		
		delete targa;
	}
	// else load as bitmap
	else 
	{
		m_IsTarga = false;
		m_HasAlphaChannel = createAlphaChannel;
		m_Handle = (HBITMAP) LoadImage(GameEngine::GetSingleton()->GetInstance(), nameRef.ToTChar(), IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);
		if (m_Handle != 0) m_Exists = true;		
	}

	if (m_IsTarga || createAlphaChannel) LoadBitInfo();
}

Bitmap::Bitmap(int IDBitmap, String const& sTypeRef, bool createAlphaChannel): m_TransparencyKey(-1), m_Opacity(100), m_Exists(false)
{
	if (sTypeRef == "BITMAP")
	{	
		m_IsTarga = false;
		m_HasAlphaChannel = createAlphaChannel;

		m_Handle = LoadBitmap(GameEngine::GetSingleton()->GetInstance(), MAKEINTRESOURCE(IDBitmap));
		
		if (m_Handle != 0) m_Exists = true;
		
		if (createAlphaChannel) LoadBitInfo();
	}
	else if (sTypeRef == "TGA")
	{
		m_IsTarga = true;
		m_HasAlphaChannel = true;

		String fileName = String("temp\\targa") + m_nr++ + ".tga";

		Extract(IDBitmap, "TGA", fileName);

		TargaLoader* targa = new TargaLoader();

		if (targa->Load(fileName.ToTChar()) == 1)
		{
			m_Handle = CreateBitmap(targa->GetWidth(), targa->GetHeight(), 1, targa->GetBPP(), (void*)targa->GetImg());
			if (m_Handle != 0) m_Exists = true;
		}
		
		delete targa;
		
		LoadBitInfo();
	}
}

void Bitmap::LoadBitInfo()
{
	BITMAPINFOHEADER bminfoheader;
	::ZeroMemory(&bminfoheader, sizeof(BITMAPINFOHEADER));
	bminfoheader.biSize        = sizeof(BITMAPINFOHEADER);
	bminfoheader.biWidth       = GetWidth();
	bminfoheader.biHeight      = GetHeight();
	bminfoheader.biPlanes      = 1;
	bminfoheader.biBitCount    = 32;
	bminfoheader.biCompression = BI_RGB;
	
	HDC windowDC = GetWindowDC(GameEngine::GetSingleton()->GetWindow());
	m_PixelsPtr = new unsigned char[this->GetWidth() * this->GetHeight() * 4];
	
	GetDIBits(windowDC, m_Handle, 0, GetHeight(), m_PixelsPtr, (BITMAPINFO*) &bminfoheader, DIB_RGB_COLORS); // load pixel info

	// premultiply if it's a targa
	if (m_IsTarga)
	{
		for (int count = 0; count < GetWidth() * GetHeight(); count++)
		{
			if (m_PixelsPtr[count * 4 + 3] < 255)
			{
				m_PixelsPtr[count * 4 + 2] = (unsigned char)((int) m_PixelsPtr[count * 4 + 2] * (int) m_PixelsPtr[count * 4 + 3] / 0xff);
				m_PixelsPtr[count * 4 + 1] = (unsigned char)((int) m_PixelsPtr[count * 4 + 1] * (int) m_PixelsPtr[count * 4 + 3] / 0xff);
				m_PixelsPtr[count * 4] = (unsigned char)((int) m_PixelsPtr[count * 4] * (int) m_PixelsPtr[count * 4 + 3] / 0xff);
			}
		}
			
		SetDIBits(windowDC, m_Handle, 0, GetHeight(), m_PixelsPtr, (BITMAPINFO*) &bminfoheader, DIB_RGB_COLORS); // save the pixel info for later manipulation
	}	
	// add alpha channel values of 255 for every pixel if bmp
	else
	{		
		for (int count = 0; count < GetWidth() * GetHeight(); count++)
		{
			m_PixelsPtr[count * 4 + 3] = 255;
		}
	}
	
	SetDIBits(windowDC, m_Handle, 0, GetHeight(), m_PixelsPtr, (BITMAPINFO*) &bminfoheader, DIB_RGB_COLORS); // save the pixel info for later manipulation
}

/*
void Bitmap::Premultiply() // Multiply R, G and B with Alpha
{
    //Note that the APIs use premultiplied alpha, which means that the red,
    //green and blue channel values in the bitmap must be premultiplied with
    //the alpha channel value. For example, if the alpha channel value is x,
    //the red, green and blue channels must be multiplied by x and divided by
    //0xff prior to the call.

    unsigned long Index,nPixels;
    unsigned char *bCur;
    short iPixelSize;

	// Set ptr to start of image
    bCur=pImage;

    // Calc number of pixels
    nPixels=iWidth*iHeight;

	// Get pixel size in bytes
    iPixelSize=iBPP/8;

    for(Index=0;Index!=nPixels;Index++)  // For each pixel
    {

        *bCur=(unsigned char)((int)*bCur* (int)*(bCur+3)/0xff);
        *(bCur+1)=(unsigned char)((int)*(bCur+1)* (int)*(bCur+3)/0xff);
        *(bCur+2)=(unsigned char)((int)*(bCur+2)* (int)*(bCur+3)/0xff);

        bCur+=iPixelSize; // Jump to next pixel
    }
}
*/

Bitmap::~Bitmap()
{
	if (HasAlphaChannel())
	{
		delete[] m_PixelsPtr;
		m_PixelsPtr = 0;
	}

	DeleteObject(m_Handle);
}

bool Bitmap::Exists()
{
	return m_Exists;
}

void Bitmap::Extract(WORD id, String sType, String sFilename)
{
	CreateDirectory(TEXT("temp\\"), NULL);

    HRSRC hrsrc = FindResource(NULL, MAKEINTRESOURCE(id), sType.ToTChar());
    HGLOBAL hLoaded = LoadResource( NULL, hrsrc);
    LPVOID lpLock =  LockResource(hLoaded);
    DWORD dwSize = SizeofResource(NULL, hrsrc);
    HANDLE hFile = CreateFile(sFilename.ToTChar(), GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    DWORD dwByteWritten;
    WriteFile(hFile, lpLock , dwSize , &dwByteWritten , NULL);
    CloseHandle(hFile);
    FreeResource(hLoaded);
} 

HBITMAP Bitmap::GetHandle()
{
    return m_Handle;
}

int Bitmap::GetWidth()
{
	if (!Exists()) return 0;

    BITMAP bm;

    GetObject(m_Handle, sizeof(bm), &bm);

    return bm.bmWidth;
}

int Bitmap::GetHeight()
{
	if (!Exists()) return 0;

	BITMAP bm;

    GetObject(m_Handle, sizeof(bm), &bm);

    return bm.bmHeight;
}

void Bitmap::SetTransparencyColor(COLORREF color) // converts transparency value to pixel-based alpha
{
	m_TransparencyKey = color;

	if (HasAlphaChannel())
	{
		BITMAPINFOHEADER bminfoheader;
		::ZeroMemory(&bminfoheader, sizeof(BITMAPINFOHEADER));
		bminfoheader.biSize        = sizeof(BITMAPINFOHEADER);
		bminfoheader.biWidth       = GetWidth();
		bminfoheader.biHeight      = GetHeight();
		bminfoheader.biPlanes      = 1;
		bminfoheader.biBitCount    = 32;
		bminfoheader.biCompression = BI_RGB;
		
		HDC windowDC = GetWindowDC(GameEngine::GetSingleton()->GetWindow());

		unsigned char* NewPixels = new unsigned char[this->GetWidth() * this->GetHeight() * 4]; // create 32 bit buffer

		for (int count = 0; count < this->GetWidth() * this->GetHeight(); ++count)
		{
			if (RGB(m_PixelsPtr[count * 4 + 2], m_PixelsPtr[count * 4 + 1], m_PixelsPtr[count * 4]) == color) // if the color of this pixel == transparency color
			{
				((int*) NewPixels)[count] = 0; // set all four values to zero, this assumes sizeof(int) == 4 on this system
												// setting values to zero means premultiplying the RGB values to an alpha of 0
			}
			else ((int*) NewPixels)[count] = ((int*) m_PixelsPtr)[count]; // copy all four values from m_Pixels to NewPixels
		}

		SetDIBits(windowDC, m_Handle, 0, GetHeight(), NewPixels, (BITMAPINFO*) &bminfoheader, DIB_RGB_COLORS); // insert pixels into bitmap

		delete[] NewPixels; //destroy buffer

		ReleaseDC(GameEngine::GetSingleton()->GetWindow(), windowDC); // release DC
	}
}

COLORREF Bitmap::GetTransparencyColor()
{
	return m_TransparencyKey;
}

void Bitmap::SetOpacity(int opacity)
{
	if (HasAlphaChannel())
	{
		if (opacity > 100) m_Opacity = 100;
		else if (opacity < 0) m_Opacity = 0;
		else m_Opacity = opacity;
	}
}

int Bitmap::GetOpacity()
{
	return m_Opacity;
}

bool Bitmap::IsTarga()
{
	return m_IsTarga;
}

bool Bitmap::HasAlphaChannel()
{
	return m_HasAlphaChannel;
}

//-----------------------------------------------------------------
// Audio methods
//-----------------------------------------------------------------

// set static datamember to zero
int Audio::m_nr = 0;

#pragma warning(disable:4311)
#pragma warning(disable:4312)
Audio::Audio(String const& nameRef) : m_Playing(false), m_Paused(false), m_MustRepeat(false), m_hWnd(0), m_Volume(100)
{	
	if (nameRef.EndsWith(".mp3") || nameRef.EndsWith(".wav") || nameRef.EndsWith(".mid"))
	{
		m_Alias = String("audio") + m_nr++;
		m_FileName = nameRef;

		Create(nameRef);
	}
}

Audio::Audio(int IDAudio, String const& typeRef) : m_Playing(false), m_Paused(false), m_MustRepeat(false), m_hWnd(0), m_Volume(100)
{
	if (typeRef == "MP3" || typeRef == "WAV" || typeRef == "MID")
	{
		m_Alias = String("audio") + m_nr++;
		m_FileName = String("temp\\") + m_Alias;

		if (typeRef == "MP3") m_FileName += ".mp3";
		else if (typeRef == "WAV") m_FileName += ".wav";
		else m_FileName += ".mid";
			
		Extract(IDAudio, typeRef, m_FileName);

		Create(m_FileName);
	}
}

void Audio::Create(const String& nameRef)
{
	TCHAR buffer[100];

	String sendString;

	if (nameRef.EndsWith(".mp3")) sendString = String("open \"") + m_FileName + "\" type mpegvideo alias " + m_Alias;
	else if (nameRef.EndsWith(".wav")) sendString = String("open \"") + m_FileName + "\" type waveaudio alias " + m_Alias;
	else if (nameRef.EndsWith(".mid")) sendString = String("open \"") + m_FileName + "\" type sequencer alias " + m_Alias;

	int result = mciSendString(sendString.ToTChar(), 0, 0, 0);	
	if (result != 0) return;
	
	sendString = String("set ") + m_Alias + " time format milliseconds";
	mciSendString(sendString.ToTChar(), 0, 0, 0);

	sendString = String("status ") + m_Alias + " length";
	mciSendString(sendString.ToTChar(), buffer, 100, 0);

	m_Duration = String(buffer).ToInteger();
	
	// Create a window to catch the MM_MCINOTIFY message with
	m_hWnd = CreateWindow(TEXT("STATIC"), TEXT(""), 0, 0, 0, 0, 0, 0, 0, GameEngine::GetSingleton()->GetInstance(), 0);
	SetWindowLong(m_hWnd, GWL_WNDPROC, (LONG) AudioProcStatic);	// set the custom message loop (subclassing)
	SetWindowLong(m_hWnd, GWL_USERDATA, (LONG) this);			// set this object as the parameter for the Proc
}

void Audio::Extract(WORD id , String sType, String sFilename)
{
	CreateDirectory(TEXT("temp\\"), NULL);

    HRSRC hrsrc = FindResource(NULL, MAKEINTRESOURCE(id), sType.ToTChar());
    HGLOBAL hLoaded = LoadResource( NULL, hrsrc);
    LPVOID lpLock =  LockResource(hLoaded);
    DWORD dwSize = SizeofResource(NULL, hrsrc);
    HANDLE hFile = CreateFile(sFilename.ToTChar(), GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    DWORD dwByteWritten;
    WriteFile(hFile, lpLock , dwSize , &dwByteWritten , NULL);
    CloseHandle(hFile);
    FreeResource(hLoaded);
} 

#pragma warning(default:4311)
#pragma warning(default:4312)

Audio::~Audio()
{
	Stop();

	String sendString = String("close ") + m_Alias;
	mciSendString(sendString.ToTChar(), 0, 0, 0);

	// release the window resources if necessary
	if (m_hWnd)
	{
		DestroyWindow(m_hWnd);
		m_hWnd = 0;
	}
}

void Audio::Play(int msecStart, int msecStop)
{
	if (!m_Playing)
	{
		m_Playing = true;
		m_Paused = false;

		if (msecStop == -1) QueuePlayCommand(msecStart);
		else QueuePlayCommand(msecStart, msecStop);
	}	
	else if (m_Paused)
	{
		m_Paused = false;

		QueueResumeCommand();
	}
}

void Audio::Pause()
{
	if (m_Playing && !m_Paused) 
	{
		m_Paused = true;

		QueuePauseCommand();
	}
}

void Audio::Stop()
{
	if (m_Playing)
	{
		m_Playing = false;
		m_Paused = false;

		QueueStopCommand();
	}
}

void Audio::QueuePlayCommand(int msecStart)
{
	QueueCommand(String("play ") + m_Alias + " from " + msecStart +  " notify");
}

void Audio::QueuePlayCommand(int msecStart, int msecStop)
{
	QueueCommand(String("play ") + m_Alias + " from " + msecStart + " to " + msecStop + " notify");
}

void Audio::QueuePauseCommand()
{
	QueueCommand(String("pause ") + m_Alias);
}

void Audio::QueueResumeCommand()
{
	QueueCommand(String("resume ") + m_Alias);
}

void Audio::QueueStopCommand()
{
	QueueCommand(String("stop ") + m_Alias);
}

void Audio::QueueVolumeCommand(int volume)
{
	QueueCommand(String("setaudio ") + m_Alias + " volume to " + volume * 10);
}

void Audio::QueueCommand(String const& commandRef)
{
	//OutputDebugString(String("Queueing command: ") + command + "\n");
	m_CommandQueue.push(commandRef);
}

void Audio::Tick()
{
	if (!m_CommandQueue.empty())
	{
		SendMCICommand(m_CommandQueue.front());
		//OutputDebugString(String("Executing queued command: ") + m_CommandQueue.front() + "\n");
		m_CommandQueue.pop();
	}
}

void Audio::SendMCICommand(String const& commandRef)
{
	int result = mciSendString(commandRef.ToTChar(), 0, 0, m_hWnd);
}

String& Audio::GetName()
{
	return m_FileName;
}
	
String& Audio::GetAlias()
{
	return m_Alias;
}

bool Audio::IsPlaying()
{
	return m_Playing;
}

bool Audio::IsPaused()
{
	return m_Paused;
}

void Audio::SwitchPlayingOff()
{
	m_Playing = false;
	m_Paused = false;
}

void Audio::SetRepeat(bool repeat)
{
	m_MustRepeat = repeat;
}

bool Audio::GetRepeat()
{
	return m_MustRepeat;
}

int Audio::GetDuration()
{
	return m_Duration;
}

void Audio::SetVolume(int volume)
{
	m_Volume = min(100, max(0, volume));	// values below 0 and above 100 are trimmed to 0 and 100, respectively

	QueueVolumeCommand(volume);
}

int Audio::GetVolume()
{
	return m_Volume;
}

bool Audio::Exists()
{
	return m_hWnd?true:false;
}

int Audio::GetType()
{
	return Caller::Audio;
}

LRESULT Audio::AudioProcStatic(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{	
	#pragma warning(disable: 4312)
	Audio* audio = reinterpret_cast<Audio*>(GetWindowLong(hWnd, GWL_USERDATA));
	#pragma warning(default: 4312)

	switch (msg)
	{		
	case MM_MCINOTIFY: // message received when an audio file has finished playing - used for repeat function

		if (wParam == MCI_NOTIFY_SUCCESSFUL && audio->IsPlaying()) 
		{
			audio->SwitchPlayingOff();

			if (audio->GetRepeat()) audio->Play();	// repeat the audio
			else audio->CallListeners();			// notify listeners that the audio file has come to an end
		}
	}
	return 0;	
}

//-----------------------------------------------------------------
// TextBox methods
//-----------------------------------------------------------------

#pragma warning(disable:4311)	
#pragma warning(disable:4312)
TextBox::TextBox(String const& textRef) : m_x(0), m_y(0), m_BgColor(RGB(255, 255, 255)), m_ForeColor(RGB(0, 0, 0)), m_BgColorBrush(0), m_Font(0), m_OldFont(0)
{
	// Create the edit box
	m_hWndEdit = CreateWindow(TEXT("EDIT"), textRef.ToTChar(), WS_BORDER | WS_CHILD | WS_CLIPSIBLINGS | WS_TABSTOP | ES_LEFT | ES_AUTOHSCROLL, 0, 0, 0, 0, GameEngine::GetSingleton()->GetWindow(), NULL, GameEngine::GetSingleton()->GetInstance(), NULL);

	// Set de nieuwe WNDPROC voor de edit box, en houd de oude bij 
	m_procOldEdit = (WNDPROC) SetWindowLong(m_hWndEdit, GWL_WNDPROC, (LONG) EditProcStatic);

	// Stel dit object in als userdata voor de statische wndproc functie van de edit box zodat deze members kan aanroepen
	SetWindowLong(m_hWndEdit, GWL_USERDATA, (LONG) this);
}

TextBox::TextBox() : m_x(0), m_y(0), m_BgColor(RGB(255, 255, 255)), m_ForeColor(RGB(0, 0, 0)), m_BgColorBrush(0), m_Font(0), m_OldFont(0)
{
	// Create the edit box
	m_hWndEdit = CreateWindow(TEXT("EDIT"), TEXT(""), WS_BORDER | WS_CHILD | WS_CLIPSIBLINGS | WS_TABSTOP | ES_LEFT | ES_AUTOHSCROLL, 0, 0, 0, 0, GameEngine::GetSingleton()->GetWindow(), NULL, GameEngine::GetSingleton()->GetInstance(), NULL);

	// Set de nieuwe WNDPROC voor de edit box, en houd de oude bij 
	m_procOldEdit = (WNDPROC) SetWindowLong(m_hWndEdit, GWL_WNDPROC, (LONG) EditProcStatic);

	// Stel dit object in als userdata voor de statische wndproc functie van de edit box zodat deze members kan aanroepen
	SetWindowLong(m_hWndEdit, GWL_USERDATA, (LONG) this);
}
#pragma warning(default:4311)
#pragma warning(default:4312)

TextBox::~TextBox()
{
	// release the background brush if necessary
	if (m_BgColorBrush != 0) 
	{
		DeleteObject(m_BgColorBrush);
		m_BgColorBrush = 0;
	}

	// release the font if necessary
	if (m_Font != 0)
	{
		SelectObject(GetDC(m_hWndEdit), m_OldFont);
		DeleteObject(m_Font);
		m_Font = m_OldFont = 0;
	}
		
	// release the window resources
	DestroyWindow(m_hWndEdit);
	m_hWndEdit = NULL;
}

void TextBox::SetBounds(int x, int y, int width, int height)
{
	m_x = x;
	m_y = y;

	MoveWindow(m_hWndEdit, x, y, width, height, true);
}

RECT TextBox::GetRect()
{
	RECT rc;

	GetClientRect(m_hWndEdit, &rc);

	rc.left += m_x;
	rc.right += m_x;
	rc.top += m_y; 
	rc.bottom += m_y;

	return rc;
}

void TextBox::SetEnabled(bool bEnable)
{
	EnableWindow(m_hWndEdit, bEnable);
}

void TextBox::Update()
{
	UpdateWindow(m_hWndEdit);
}

void TextBox::Show()
{
	// Show and update the edit box
	ShowWindow(m_hWndEdit, SW_SHOW);
	UpdateWindow(m_hWndEdit);
}

void TextBox::Hide()
{
	// Show and update the edit box
	ShowWindow(m_hWndEdit, SW_HIDE);
	UpdateWindow(m_hWndEdit);
}

String TextBox::GetText()
{
	int textLength = (int) SendMessage(m_hWndEdit, (UINT) WM_GETTEXTLENGTH, 0, 0);
	
	TCHAR* bufferPtr = new TCHAR[textLength + 1];

	SendMessage(m_hWndEdit, (UINT) WM_GETTEXT, (WPARAM) textLength + 1, (LPARAM) bufferPtr);

	String newString(bufferPtr);

	delete[] bufferPtr;

	return newString;
}

void TextBox::SetText(String const& textRef)
{
	SendMessage(m_hWndEdit, WM_SETTEXT, 0, (LPARAM) textRef.ToTChar());
}

void TextBox::SetFont(String const& fontNameRef, bool bold, bool italic, bool underline, int size)
{
	LOGFONT ft;

	_tcscpy_s(ft.lfFaceName, sizeof(ft.lfFaceName) / sizeof(TCHAR), fontNameRef.ToTChar());
	ft.lfStrikeOut = 0;
	ft.lfUnderline = underline?1:0;
	ft.lfHeight = size;
    ft.lfEscapement = 0;
	ft.lfWeight = bold?FW_BOLD:0;
	ft.lfItalic = italic?1:0;

	// clean up if another custom font was already in place
	if (m_Font != 0) { DeleteObject(m_Font); }

	// create the new font. The WM_CTLCOLOREDIT message will set the font when the textbox is about to redraw
    m_Font = CreateFontIndirect(&ft);

	// redraw the textbox
	InvalidateRect(m_hWndEdit, NULL, true);
}

void TextBox::SetForecolor( COLORREF color )
{
	m_ForeColor = color;
	
	// redraw the textbox
	InvalidateRect(m_hWndEdit, NULL, true);
}

void TextBox::SetBackcolor( COLORREF color )
{
	m_BgColor = color;
	
	if (m_BgColorBrush != 0) DeleteObject(m_BgColorBrush);
	m_BgColorBrush = CreateSolidBrush( color );
	
	// redraw the textbox
	InvalidateRect(m_hWndEdit, NULL, true);
}

COLORREF TextBox::GetForecolor()
{
	return m_ForeColor;
}

COLORREF TextBox::GetBackcolor()
{
	return m_BgColor;
}

HBRUSH TextBox::GetBackcolorBrush()
{
	return m_BgColorBrush;
}

LRESULT TextBox::EditProcStatic(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	#pragma warning(disable: 4312)
	return reinterpret_cast<TextBox*>(GetWindowLong(hWnd, GWL_USERDATA))->EditProc(hWnd, msg, wParam, lParam);
	#pragma warning(default: 4312)
}

LRESULT TextBox::EditProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{		
	case WM_CTLCOLOREDIT:
		SetBkColor((HDC) wParam, GetBackcolor() );
		SetTextColor((HDC) wParam, GetForecolor() );
		if (m_Font != 0) 
		{
			if (m_OldFont == 0) m_OldFont = (HFONT) SelectObject((HDC) wParam, m_Font);
			else SelectObject((HDC) wParam, m_Font);
		}
		return (LRESULT) GetBackcolorBrush();

	case WM_CHAR: 
		if (wParam == VK_TAB) return 0;
		if (wParam == VK_RETURN) return 0;
		break;

	case WM_KEYDOWN :
		switch (wParam)
		{
		case VK_TAB:
			if (GameEngine::GetSingleton()->IsKeyDown(VK_SHIFT)) GameEngine::GetSingleton()->TabPrevious(hWnd);
			else GameEngine::GetSingleton()->TabNext(hWnd);
			return 0;
		case VK_ESCAPE:
			SetFocus(GetParent(hWnd));
			return 0;
		case VK_RETURN:
			//if (m_Target) result = m_Target->CallAction(this);
			CallListeners();
			break;
		}
	}
	return CallWindowProc(m_procOldEdit, hWnd, msg, wParam, lParam);
}


//-----------------------------------------------------------------
// Button methods
//-----------------------------------------------------------------

#pragma warning(disable:4311)
#pragma warning(disable:4312)
Button::Button(String const& textRef) : m_x(0), m_y(0), m_Armed(false), m_Font(0), m_OldFont(0)
{
	// Create the button object
	m_hWndButton = CreateWindow(TEXT("BUTTON"), textRef.ToTChar(), WS_BORDER | WS_CHILD | WS_CLIPSIBLINGS | WS_TABSTOP | BS_PUSHBUTTON, 0, 0, 0, 0, GameEngine::GetSingleton()->GetWindow(), NULL, GameEngine::GetSingleton()->GetInstance(), NULL);

	// Set de new WNDPROC for the button, and store the old one
	m_procOldButton = (WNDPROC) SetWindowLong(m_hWndButton, GWL_WNDPROC, (LONG) ButtonProcStatic);

	// Store 'this' as data for the Button object so that the static PROC can call the member proc
	SetWindowLong(m_hWndButton, GWL_USERDATA, (LONG) this);
}

Button::Button() : m_x(0), m_y(0), m_Armed(false), m_Font(0), m_OldFont(0)
{
	// Create the button object
	m_hWndButton = CreateWindow(TEXT("BUTTON"), TEXT(""), WS_BORDER | WS_CHILD | WS_CLIPSIBLINGS | WS_TABSTOP | BS_PUSHBUTTON, 0, 0, 0, 0, GameEngine::GetSingleton()->GetWindow(), NULL, GameEngine::GetSingleton()->GetInstance(), NULL);

	// Set de new WNDPROC for the button, and store the old one
	m_procOldButton = (WNDPROC) SetWindowLong(m_hWndButton, GWL_WNDPROC, (LONG) ButtonProcStatic);

	// Store 'this' as data for the Button object so that the static PROC can call the member proc
	SetWindowLong(m_hWndButton, GWL_USERDATA, (LONG) this);
}
#pragma warning(default:4311)
#pragma warning(default:4312)

Button::~Button()
{
	// release the font if necessary
	if (m_Font != 0)
	{
		SelectObject(GetDC(m_hWndButton), m_OldFont);
		DeleteObject(m_Font);
		m_Font = m_OldFont = 0;
	}
		
	// release the window resource
	DestroyWindow(m_hWndButton);
	m_hWndButton = NULL;	
}

void Button::SetBounds(int x, int y, int width, int height)
{
	m_x = x;
	m_y = y;

	MoveWindow(m_hWndButton, x, y, width, height, true);
}

RECT Button::GetRect()
{
	RECT rc;

	GetClientRect(m_hWndButton, &rc);
	
	rc.left += m_x;
	rc.right += m_x;
	rc.top += m_y; 
	rc.bottom += m_y;

	return rc;
}

void Button::SetEnabled(bool bEnable)
{
	EnableWindow(m_hWndButton, bEnable);
}

void Button::Update()
{
	UpdateWindow(m_hWndButton);
}

void Button::Show()
{
	// Show and update the button
	ShowWindow(m_hWndButton, SW_SHOW);
	UpdateWindow(m_hWndButton);
}

void Button::Hide()
{
	// Show and update the button
	ShowWindow(m_hWndButton, SW_HIDE);
	UpdateWindow(m_hWndButton);
}

String Button::GetText()
{
	int textLength = (int) SendMessage(m_hWndButton, (UINT) WM_GETTEXTLENGTH, 0, 0);
	
	TCHAR* bufferPtr = new TCHAR[textLength + 1];

	SendMessage(m_hWndButton, (UINT) WM_GETTEXT, (WPARAM) textLength + 1, (LPARAM) bufferPtr);

	String newString(bufferPtr);

	delete[] bufferPtr;

	return newString;
}

void Button::SetText(String const& textRef)
{
	SendMessage(m_hWndButton, WM_SETTEXT, 0, (LPARAM) textRef.ToTChar());
}

void Button::SetFont(String const& fontNameRef, bool bold, bool italic, bool underline, int size)
{
	LOGFONT ft;

	_tcscpy_s(ft.lfFaceName, sizeof(ft.lfFaceName) / sizeof(TCHAR), fontNameRef.ToTChar());
	ft.lfStrikeOut = 0;
	ft.lfUnderline = underline?1:0;
	ft.lfHeight = size;
    ft.lfEscapement = 0;
	ft.lfWeight = bold?FW_BOLD:0;
	ft.lfItalic = italic?1:0;

	// clean up if another custom font was already in place
	if (m_Font != 0) { DeleteObject(m_Font); }

	// create the new font. The WM_CTLCOLOREDIT message will set the font when the button is about to redraw
    m_Font = CreateFontIndirect(&ft);

	// redraw the button
	InvalidateRect(m_hWndButton, NULL, true);
}

LRESULT Button::ButtonProcStatic(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	#pragma warning(disable: 4312)
	return reinterpret_cast<Button*>(GetWindowLong(hWnd, GWL_USERDATA))->ButtonProc(hWnd, msg, wParam, lParam);
	#pragma warning(default: 4312)
}

LRESULT Button::ButtonProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
	case WM_CTLCOLOREDIT:
		if (m_Font != 0) 
		{
			if (m_OldFont == 0) m_OldFont = (HFONT) SelectObject((HDC) wParam, m_Font);
			else SelectObject((HDC) wParam, m_Font);
		}
		return 0;

	case WM_CHAR: 
		if (wParam == VK_TAB) return 0;
		if (wParam == VK_RETURN) return 0;
		break;

	case WM_KEYDOWN :
		switch (wParam)
		{
		case VK_TAB:			
			if (GameEngine::GetSingleton()->IsKeyDown(VK_SHIFT)) GameEngine::GetSingleton()->TabPrevious(hWnd);
			else GameEngine::GetSingleton()->TabNext(hWnd);
			return 0;
		case VK_ESCAPE:
			SetFocus(GetParent(hWnd));
			return 0;
		case VK_SPACE:
			//if (m_Target) result = m_Target->CallAction(this);
			CallListeners();
		}
		break;
	case WM_LBUTTONDOWN :
	case WM_LBUTTONDBLCLK:					// clicking fast will throw LBUTTONDBLCLK's as well as LBUTTONDOWN's, you need to capture both to catch all button clicks
		m_Armed = true;
		break;
	case WM_LBUTTONUP :
		if (m_Armed)
		{
			RECT rc;
			POINT pt;
			GetWindowRect(hWnd, &rc);
			GetCursorPos(&pt);

			//if (PtInRect(&rc, pt) && m_Target) result = m_Target->CallAction(this);
			if (PtInRect(&rc, pt)) CallListeners();

			m_Armed = false;
		}
	}
	return CallWindowProc(m_procOldButton, hWnd, msg, wParam, lParam);
}

//-----------------------------------------------------------------
// Timer methods
//-----------------------------------------------------------------

Timer::Timer(int msec, Callable* targetPtr) : m_IsRunning(false)
{
	m_Delay = msec;

	AddActionListener(targetPtr);
}

Timer::~Timer()
{
	if (m_IsRunning) Stop(); // stop closes the handle

	// no objects to delete
}

void Timer::Start()
{
	if (m_IsRunning == false)
	{
		CreateTimerQueueTimer(&m_TimerHandle, NULL, TimerProcStatic, (void*) this, m_Delay, m_Delay, WT_EXECUTEINTIMERTHREAD);	
		m_IsRunning = true;
	}
}

void Timer::Stop()
{	
	if (m_IsRunning == true)
	{
		DeleteTimerQueueTimer(NULL, m_TimerHandle, NULL);  
		//CloseHandle (m_TimerHandle);		DeleteTimerQueueTimer automatically closes the handle? MSDN Documentation seems to suggest this
		
		m_IsRunning = false;
	}
}

bool Timer::IsRunning()
{
	return m_IsRunning;
}

void Timer::SetDelay(int msec)
{
	m_Delay = max(msec, 1); // timer will not accept values less than 1 msec

	if (m_IsRunning)
	{
		Stop();
		Start();
	}
}

int Timer::GetDelay()
{
	return m_Delay;
}

void CALLBACK Timer::TimerProcStatic(void* lpParameter, BOOLEAN TimerOrWaitFired)
{
	Timer* timer = reinterpret_cast<Timer*>(lpParameter);

	//if (timer->m_IsRunning) timer->m_Target->CallAction(timer);
	if (timer->m_IsRunning) timer->CallListeners();
}

//-----------------------------------------------------------------
// String methods
// CBUGFIX
//-----------------------------------------------------------------

String::String(wchar_t const* wideTextPtr)
{		
	m_Length = (int) wcslen(wideTextPtr) + 1; // include room for null terminator
	m_TextPtr = new TCHAR[m_Length]; 

	if (sizeof(TCHAR) == 2) _tcscpy_s(m_TextPtr, m_Length, (TCHAR*) wideTextPtr);
	else WideCharToMultiByte(CP_ACP, 0, wideTextPtr, -1, (char*) m_TextPtr, m_Length, NULL, NULL);
}

String::String(char const* singleTextPtr)
{
	m_Length = (int) strlen(singleTextPtr) + 1; // inlude room for null terminator

	m_TextPtr = new TCHAR[m_Length]; 
	
	if (sizeof(TCHAR) == 1) strcpy_s((char*) m_TextPtr, m_Length, singleTextPtr);
	else MultiByteToWideChar(CP_ACP, 0, singleTextPtr, -1, (wchar_t*) m_TextPtr, m_Length * 2);
}

String::String(String const& sRef)
{   
	m_Length = sRef.GetLength() + 1; // include room for null terminator
	m_TextPtr = new TCHAR[m_Length];

	_tcscpy_s(m_TextPtr, m_Length, sRef.ToTChar());
}

String::String(wchar_t character)
{
	m_Length = 2; // include room for null terminator
	m_TextPtr = new TCHAR[m_Length];

	m_TextPtr[0] = character;
	m_TextPtr[1] = '\0';
}

String::String(char character)
{
	m_Length = 2; // include room for null terminator
	m_TextPtr = new TCHAR[m_Length];

	m_TextPtr[0] = character;
	m_TextPtr[1] = '\0';
}

String::~String()
{
	delete m_TextPtr;
	m_TextPtr = 0;
}

String& String::operator=(String const& sRef)
{
	if (this != &sRef) // beware of self assignment: s = s
	{
		delete m_TextPtr;
		m_Length = sRef.GetLength() + 1;
		m_TextPtr = new TCHAR[m_Length];
		_tcscpy_s(m_TextPtr, m_Length, sRef.ToTChar());
	}
	return *this;
}
	
String& String::operator+=(String const& sRef)
{
	m_Length = this->GetLength() + sRef.GetLength() + 1;
	
	TCHAR* buffer = new TCHAR[m_Length];

	_tcscpy_s(buffer, m_Length, m_TextPtr);
	_tcscat_s(buffer, m_Length, sRef.m_TextPtr);

	delete m_TextPtr;
	m_TextPtr = buffer;

	return *this;
}

String& String::operator+=(wchar_t* wideTextPtr)
{
	return *this += String(wideTextPtr);
}

String& String::operator+=(char* singleTextPtr)
{
	return *this += String(singleTextPtr);
}

String& String::operator+=(int number)
{	
	char buffer[65]; // an int will never take more than 65 characters (int64 is max 20 characters) (QUESTIONMARK: why use more then 20 then??)

	_itoa_s(number, buffer, sizeof(buffer), 10);

	return *this += String(buffer);
}

String& String::operator+=(size_t number)
{	
	char buffer[65]; // an int will never take more than 65 characters (int64 is max 20 characters)

	_ultoa_s((unsigned long) number, buffer, sizeof(buffer), 10);
	
	return *this += String(buffer);
}

String& String::operator+=(double number)
{
	char buffer[_CVTBUFSIZE];

	_gcvt_s(buffer, _CVTBUFSIZE, number, 40); // max 40 digits

	if (number == (int) number) // _gcvt_s forgets a trailing 0 when there are no significant digits behind the comma, so add it manually
	{
		size_t length = strlen(buffer);
		buffer[length] = '0';
		buffer[length+1] = '\0';
	}
 
	return *this += String(buffer);
}

String& String::operator+=(wchar_t character)
{		
	return *this += String(character);
}

String& String::operator+=(char character)
{		
	return *this += String(character);
}

String String::operator+(String const& sRef)
{
	String newString = *this;

	newString += sRef;

	return newString;
}

String String::operator+(wchar_t* wideTextPtr)
{
	String newString = *this;

	newString += wideTextPtr;

	return newString;
}

String String::operator+(char* singleTextPtr)
{
	String newString = *this;

	newString += singleTextPtr;

	return newString;
}

String String::operator+(int number)
{
	String newString = *this;

	newString += number;

	return newString;
}

String String::operator+(size_t number)
{
	String newString = *this;

	newString += number;

	return newString;
}

String String::operator+(double number)
{ 
	String newString = *this;

	newString += number;

	return newString;
}

String String::operator+(wchar_t character)
{ 
	String newString = *this;

	newString += character;

	return newString;
}

String String::operator+(char character)
{ 
	String newString = *this;

	newString += character;

	return newString;
}

bool String::operator==(String const& sRef)
{
	return this->Equals(sRef);
}

bool String::operator==(String const& sRef) const
{
	return this->Equals(sRef);
}

TCHAR String::CharAt(int index) const
{
	if (index > this->GetLength() - 1) return 0;
	//QUESTIONMARK
	return m_TextPtr[index];
}

String String::Replace(TCHAR oldChar, TCHAR newChar) const
{
	String newString = *this;

	for (int count = 0; count < newString.GetLength(); count++)
	{
		if (newString.m_TextPtr[count] == oldChar) newString.m_TextPtr[count] = newChar;
	}
	
	return newString;
}

String String::SubString(int index) const
{
	if (index > this->GetLength() - 1) return String(""); 

	return String(m_TextPtr + index);
}

String String::SubString(int index, int length) const
{
	if (index + length - 1 > this->GetLength() - 1) return String(""); 

	String newString = *this;
	newString.m_TextPtr[index + length] = TEXT('\0');

	return String(newString.m_TextPtr + index);
}

String String::ToLowerCase() const
{
	String newString = *this;	

	for (int count = 0; count < newString.GetLength(); count++)
	{
		TCHAR character = newString.m_TextPtr[count];

		if (character < 91 && character > 64) newString.m_TextPtr[count] += 32;
	}

	return newString;
}

String String::ToUpperCase() const
{
	String newString = *this;	

	for (int count = 0; count < newString.GetLength(); count++)
	{
		TCHAR character = newString.m_TextPtr[count];

		if (character < 123 && character > 96) newString.m_TextPtr[count] -= 32;
	}

	return newString;
}


String String::Trim() const
{
	int beginIndex = 0;
	int endIndex = this->GetLength() - 1;

	while (m_TextPtr[beginIndex] == TEXT(' ') && m_TextPtr[beginIndex] != TEXT('\0')) ++beginIndex;
	while (m_TextPtr[endIndex] == TEXT(' ') && endIndex > 0) --endIndex;

	return this->SubString(beginIndex, endIndex - beginIndex + 1);
}

int String::IndexOf(TCHAR character) const
{
	int index = 0;

	while (m_TextPtr[index] != character && m_TextPtr[index] != TEXT('\0')) ++index;

	if (m_TextPtr[index] == character) return index;
	else return -1;
}

int String::LastIndexOf(TCHAR character) const
{
	int index = this->GetLength() - 1;

	while (m_TextPtr[index] != character && index > 0) --index;

	if (m_TextPtr[index] == character) return index;
	else return -1;
}

bool String::StartsWith(String const& sRef) const
{
	// return false if 2nd string is longer than 1st string 
	if (this->GetLength() < sRef.GetLength()) return false;

	// check individual characters
	bool result = true;
	int index = 0;
	int max = sRef.GetLength();

	while (index < max && result)
	{
		if (m_TextPtr[index] != sRef.m_TextPtr[index]) result = false;
		else ++index;
	}

	return result;
}

bool String::EndsWith(String const& sRef) const
{
	// return false if 2nd string is longer than 1st string 
	if (this->GetLength() < sRef.GetLength()) return false;

	String temp = this->SubString(this->GetLength() - sRef.GetLength());
	
	return sRef.Equals(temp);
}

int String::GetLength() const
{
	return m_Length - 1; // don't include the null terminator when asked how many characters are in the string
}

bool String::Equals(String const& sRef) const
{
	if (sRef.GetLength() != this->GetLength()) return false;

	return _tcscmp(this->ToTChar(), sRef.ToTChar())?false:true; // return true if cmp returns 0, false if not
}

int String::ToInteger() const
{
	return _tstoi(this->ToTChar());
}

double String::ToDouble() const
{
	return _tcstod(this->ToTChar(), 0);
}

TCHAR* String::ToTChar() const
{
	return m_TextPtr;
}

//-----------------------------------------------------------------
// Targa loader code
//-----------------------------------------------------------------
 
#define IMG_OK              0x1
#define IMG_ERR_NO_FILE     0x2
#define IMG_ERR_MEM_FAIL    0x4
#define IMG_ERR_BAD_FORMAT  0x8
#define IMG_ERR_UNSUPPORTED 0x40
 
TargaLoader::TargaLoader()
{ 
	pImage=pPalette=pData=NULL;
	iWidth=iHeight=iBPP=bEnc=0;
	lImageSize=0;
}
 
TargaLoader::~TargaLoader()
{
	if(pImage)
	{
		delete [] pImage;
		pImage=NULL;
	}
 
	if(pPalette)
	{
		delete [] pPalette;
		pPalette=NULL;
	}
 
	if(pData)
	{
		delete [] pData;
		pData=NULL;
	}
}
 
int TargaLoader::Load(TCHAR* szFilename)
{
	using namespace std;
	ifstream fIn;
	unsigned long ulSize;
	int iRet;
 
	// Clear out any existing image and palette
	if(pImage)
    {
		delete [] pImage;
		pImage=NULL;
    }
 
	if(pPalette)
    {
		delete [] pPalette;
		pPalette=NULL;
    }
 
	// Open the specified file
	//fIn.open(szFilename,ios::binary);
	fIn.open(szFilename,ios::binary);
    
	if(fIn==NULL) return IMG_ERR_NO_FILE;
 
	// Get file size
	fIn.seekg(0,ios_base::end);
	ulSize= (unsigned long) fIn.tellg();
	fIn.seekg(0,ios_base::beg);
 
	// Allocate some space
	// Check and clear pDat, just in case
	if(pData) delete [] pData; 
 
	pData=new unsigned char[ulSize];
 
	if(pData==NULL)
    {
		fIn.close();
		return IMG_ERR_MEM_FAIL;
	}
 
	// Read the file into memory
	fIn.read((char*)pData,ulSize);
 
	fIn.close();
 
	// Process the header
	iRet=ReadHeader();
 
	if(iRet!=IMG_OK) return iRet;
 
	switch(bEnc)
	{
    case 1: // Raw Indexed
		// Check filesize against header values
        if((lImageSize+18+pData[0]+768)>ulSize) return IMG_ERR_BAD_FORMAT;
 
		// Double check image type field
		if(pData[1]!=1) return IMG_ERR_BAD_FORMAT;
 
		// Load image data
		iRet=LoadRawData();
        
		if(iRet!=IMG_OK) return iRet;
 
		// Load palette
        iRet=LoadTgaPalette();
        
		if(iRet!=IMG_OK) return iRet;
 
       break;
 
    case 2: // Raw RGB
		// Check filesize against header values
		if((lImageSize+18+pData[0])>ulSize) return IMG_ERR_BAD_FORMAT;
 
		// Double check image type field
        if(pData[1]!=0) return IMG_ERR_BAD_FORMAT;
 
		// Load image data
		iRet=LoadRawData();
        
		if(iRet!=IMG_OK) return iRet;
 
		//BGRtoRGB(); // Convert to RGB
		break;
 
    case 9: // RLE Indexed
      	// Double check image type field
        if(pData[1]!=1) return IMG_ERR_BAD_FORMAT;
 
		// Load image data
		iRet=LoadTgaRLEData();
			
		if(iRet!=IMG_OK) return iRet;
 
		// Load palette
		iRet=LoadTgaPalette();
        
		if(iRet!=IMG_OK) return iRet;
 
		break;
	
	case 10: // RLE RGB
       // Double check image type field
       if(pData[1]!=0) return IMG_ERR_BAD_FORMAT;
 
       // Load image data
       iRet=LoadTgaRLEData();
        
	   if(iRet!=IMG_OK) return iRet;
 
       //BGRtoRGB(); // Convert to RGB
       break;
 
	default:
		return IMG_ERR_UNSUPPORTED;
    }
 
	// Check flip bit
	if((pData[17] & 0x20)==0) FlipImg();
 
	// Release file memory
	delete [] pData;
	pData=NULL;

	return IMG_OK;
}
 
int TargaLoader::ReadHeader() // Examine the header and populate our class attributes
{
	short ColMapStart,ColMapLen;
	short x1,y1,x2,y2;
 
	if(pData==NULL)
		return IMG_ERR_NO_FILE;
 
	if(pData[1]>1)    // 0 (RGB) and 1 (Indexed) are the only types we know about
		return IMG_ERR_UNSUPPORTED;
 
	bEnc=pData[2];     // Encoding flag  1 = Raw indexed image
                      //                2 = Raw RGB
                      //                3 = Raw greyscale
                      //                9 = RLE indexed
                      //               10 = RLE RGB
                      //               11 = RLE greyscale
                      //               32 & 33 Other compression, indexed
 
	if(bEnc>11)       // We don't want 32 or 33
		return IMG_ERR_UNSUPPORTED;
 
 
	// Get palette info
	memcpy(&ColMapStart,&pData[3],2);
	memcpy(&ColMapLen,&pData[5],2);
 
	// Reject indexed images if not a VGA palette (256 entries with 24 bits per entry)
	if(pData[1]==1) // Indexed
    {
		if(ColMapStart!=0 || ColMapLen!=256 || pData[7]!=24) return IMG_ERR_UNSUPPORTED;
    }
 
	// Get image window and produce width & height values
	memcpy(&x1,&pData[8],2);
	memcpy(&y1,&pData[10],2);
	memcpy(&x2,&pData[12],2);
	memcpy(&y2,&pData[14],2);
 
	iWidth=(x2-x1);
	iHeight=(y2-y1);
 
	if(iWidth<1 || iHeight<1) return IMG_ERR_BAD_FORMAT;
 
	// Bits per Pixel
	iBPP=pData[16];
 
	// Check flip / interleave byte
	if(pData[17]>32) // Interleaved data
		return IMG_ERR_UNSUPPORTED;
 
	// Calculate image size
	lImageSize=(iWidth * iHeight * (iBPP/8));
 
	return IMG_OK;
}
 
int TargaLoader::LoadRawData() // Load uncompressed image data
{
	short iOffset;
 
	if(pImage) // Clear old data if present
		delete [] pImage;
 
	pImage=new unsigned char[lImageSize];
 
	if(pImage==NULL) return IMG_ERR_MEM_FAIL;
 
	iOffset=pData[0]+18; // Add header to ident field size
 
	if(pData[1]==1) // Indexed images
		iOffset+=768;  // Add palette offset
 
	memcpy(pImage,&pData[iOffset],lImageSize);
 
	return IMG_OK;
}
 
int TargaLoader::LoadTgaRLEData() // Load RLE compressed image data
{
	short iOffset,iPixelSize;
	unsigned char *pCur;
	unsigned long Index=0;
	unsigned char bLength,bLoop;
 
	// Calculate offset to image data
	iOffset=pData[0]+18;
 
	// Add palette offset for indexed images
	if(pData[1]==1) iOffset+=768; 
 
	// Get pixel size in bytes
	iPixelSize=iBPP/8;
 
	// Set our pointer to the beginning of the image data
	pCur=&pData[iOffset];
 
	// Allocate space for the image data
	if(pImage!=NULL) delete [] pImage;
 
	pImage=new unsigned char[lImageSize];
 
	if(pImage==NULL) return IMG_ERR_MEM_FAIL;
 
	// Decode
	while(Index<lImageSize) 
    {
		if(*pCur & 0x80) // Run length chunk (High bit = 1)
		{
			bLength=*pCur-127; // Get run length
			pCur++;            // Move to pixel data  
 
			// Repeat the next pixel bLength times
			for(bLoop=0;bLoop!=bLength;++bLoop,Index+=iPixelSize)
			memcpy(&pImage[Index],pCur,iPixelSize);
  
			pCur+=iPixelSize; // Move to the next descriptor chunk
		}
		else // Raw chunk
		{
			bLength=*pCur+1; // Get run length
			pCur++;          // Move to pixel data
 
			// Write the next bLength pixels directly
			for(bLoop=0;bLoop!=bLength;++bLoop,Index+=iPixelSize,pCur+=iPixelSize)
			memcpy(&pImage[Index],pCur,iPixelSize);
		}
    }
 
	return IMG_OK;
}
 
int TargaLoader::LoadTgaPalette() // Load a 256 color palette
{
	unsigned char bTemp;
	short iIndex,iPalPtr;
  
	// Delete old palette if present
	if(pPalette)
    {
		delete [] pPalette;
		pPalette=NULL;
    }
 
	// Create space for new palette
	pPalette=new unsigned char[768];
 
	if(pPalette==NULL) return IMG_ERR_MEM_FAIL;
 
	// VGA palette is the 768 bytes following the header
	memcpy(pPalette,&pData[pData[0]+18],768);
 
	// Palette entries are BGR ordered so we have to convert to RGB
	for(iIndex=0,iPalPtr=0;iIndex!=256;++iIndex,iPalPtr+=3)
    {
		bTemp=pPalette[iPalPtr];               // Get Blue value
		pPalette[iPalPtr]=pPalette[iPalPtr+2]; // Copy Red to Blue
		pPalette[iPalPtr+2]=bTemp;             // Replace Blue at the end
	}
 
	return IMG_OK;
} 
 
void TargaLoader::BGRtoRGB() // Convert BGR to RGB (or back again)
{
	unsigned long Index,nPixels;
	unsigned char *bCur;
	unsigned char bTemp;
	short iPixelSize;
 
	// Set ptr to start of image
	bCur=pImage;
 
	// Calc number of pixels
	nPixels=iWidth*iHeight;
 
	// Get pixel size in bytes
	iPixelSize=iBPP/8;
 
	for(Index=0;Index!=nPixels;Index++)  // For each pixel
    {
		bTemp=*bCur;      // Get Blue value
		*bCur=*(bCur+2);  // Swap red value into first position
		*(bCur+2)=bTemp;  // Write back blue to last position
 
		bCur+=iPixelSize; // Jump to next pixel
    }
}
 
void TargaLoader::FlipImg() // Flips the image vertically (Why store images upside down?)
{
	unsigned char bTemp;
	unsigned char *pLine1, *pLine2;
	int iLineLen,iIndex;
 
	iLineLen=iWidth*(iBPP/8);
	pLine1=pImage;
	pLine2=&pImage[iLineLen * (iHeight - 1)];
 
	for( ;pLine1<pLine2;pLine2-=(iLineLen*2))
    {
		for(iIndex=0;iIndex!=iLineLen;pLine1++,pLine2++,iIndex++)
		{
			bTemp=*pLine1;
			*pLine1=*pLine2;
			*pLine2=bTemp;       
		}
	}  
}
  
int TargaLoader::GetBPP() 
{
	return iBPP;
}
 
int TargaLoader::GetWidth()
{
	return iWidth;
}
 
int TargaLoader::GetHeight()
{
	return iHeight;
}
 
unsigned char* TargaLoader::GetImg()
{
	return pImage;
}
 
unsigned char* TargaLoader::GetPalette()
{
	return pPalette;
}

//-----------------------------------------------------------------
// OutputDebugString functions
//-----------------------------------------------------------------

void OutputDebugString(String const& textRef)
{
	OutputDebugString(textRef.ToTChar());
}


//---------------------------
// HitRegion methods
//---------------------------
HitRegion::HitRegion() : m_HitRegion(0)
{
	// nothing to create
}

HitRegion::~HitRegion()
{
	if (m_HitRegion)
		DeleteObject(m_HitRegion);
}


bool HitRegion::Create(int type, int x, int y, int width, int height)
{
	if (m_HitRegion) DeleteObject(m_HitRegion); 

	if (type == HitRegion::Ellipse)
		m_HitRegion = CreateEllipticRgn(x, y, x + width, y + height);
	else
		m_HitRegion = CreateRectRgn(x, y, x + width, y + height);

	return true;
}

bool HitRegion::Create(int type, POINT* pointsArr, int numberOfPoints)
{
	if (m_HitRegion) DeleteObject(m_HitRegion);

	m_HitRegion = CreatePolygonRgn(pointsArr, numberOfPoints, WINDING);

	return true;
}	

bool HitRegion::Create(int type, Bitmap* bmpPtr, COLORREF cTransparent, COLORREF cTolerance)
{
	if (!bmpPtr->Exists()) return false;

	HBITMAP hBitmap = bmpPtr->GetHandle();

	if (!hBitmap) return false;

	if (m_HitRegion) DeleteObject(m_HitRegion);

	// for some reason, the BitmapToRegion function has R and B switched. Flipping the colors to get the right result.
	COLORREF flippedTransparent = RGB(GetBValue(cTransparent), GetGValue(cTransparent), GetRValue(cTransparent));
	COLORREF flippedTolerance = RGB(GetBValue(cTolerance), GetGValue(cTolerance), GetRValue(cTolerance));

	m_HitRegion = BitmapToRegion(hBitmap, flippedTransparent, flippedTolerance);

	return (m_HitRegion?true:false);
}	

//	BitmapToRegion :	Create a region from the "non-transparent" pixels of a bitmap
//	Author :		Jean-Edouard Lachand-Robert (http://www.geocities.com/Paris/LeftBank/1160/resume.htm), June 1998
//  Some modifications: Kevin Hoefman, Febr 2007
HRGN HitRegion::BitmapToRegion(HBITMAP hBmp, COLORREF cTransparentColor, COLORREF cTolerance)
{
	HRGN hRgn = NULL;

	if (hBmp)
	{
		// Create a memory DC inside which we will scan the bitmap content
		HDC hMemDC = CreateCompatibleDC(NULL);

		if (hMemDC)
		{
			// Get bitmap siz
			BITMAP bm;
			GetObject(hBmp, sizeof(bm), &bm);

			// Create a 32 bits depth bitmap and select it into the memory DC
			BITMAPINFOHEADER RGB32BITSBITMAPINFO = {
					sizeof(BITMAPINFOHEADER),	// biSize
					bm.bmWidth,					// biWidth;
					bm.bmHeight,				// biHeight;
					1,							// biPlanes;
					32,							// biBitCount
					BI_RGB,						// biCompression;
					0,							// biSizeImage;
					0,							// biXPelsPerMeter;
					0,							// biYPelsPerMeter;
					0,							// biClrUsed;
					0							// biClrImportant;
			};
			VOID * pbits32;
			HBITMAP hbm32 = CreateDIBSection(hMemDC, (BITMAPINFO *)&RGB32BITSBITMAPINFO, DIB_RGB_COLORS, &pbits32, NULL, 0);

			if (hbm32)
			{
				HBITMAP holdBmp = (HBITMAP)SelectObject(hMemDC, hbm32);

				// Create a DC just to copy the bitmap into the memory D
				HDC hDC = CreateCompatibleDC(hMemDC);

				if (hDC)
				{
					// Get how many bytes per row we have for the bitmap bits (rounded up to 32 bits
					BITMAP bm32;
					GetObject(hbm32, sizeof(bm32), &bm32);
					while (bm32.bmWidthBytes % 4)
						bm32.bmWidthBytes++;

					// Copy the bitmap into the memory D
					HBITMAP holdBmp = (HBITMAP)SelectObject(hDC, hBmp);
					BitBlt(hMemDC, 0, 0, bm.bmWidth, bm.bmHeight, hDC, 0, 0, SRCCOPY);

					// For better performances, we will use the ExtCreateRegion() function to create the
					// region. This function take a RGNDATA structure on entry. We will add rectangles b
					// amount of ALLOC_UNIT number in this structure
					#define ALLOC_UNIT	100
					DWORD maxRects = ALLOC_UNIT;
					HANDLE hData = GlobalAlloc(GMEM_MOVEABLE, sizeof(RGNDATAHEADER) + (sizeof(RECT) * maxRects));
					RGNDATA *pData = (RGNDATA *)GlobalLock(hData);
					pData->rdh.dwSize = sizeof(RGNDATAHEADER);
					pData->rdh.iType = RDH_RECTANGLES;
					pData->rdh.nCount = pData->rdh.nRgnSize = 0;
					SetRect(&pData->rdh.rcBound, MAXLONG, MAXLONG, 0, 0);

					// Keep on hand highest and lowest values for the "transparent" pixel
					BYTE lr = GetRValue(cTransparentColor);
					BYTE lg = GetGValue(cTransparentColor);
					BYTE lb = GetBValue(cTransparentColor);
					BYTE hr = min(0xff, lr + GetRValue(cTolerance));
					BYTE hg = min(0xff, lg + GetGValue(cTolerance));
					BYTE hb = min(0xff, lb + GetBValue(cTolerance));

					// Scan each bitmap row from bottom to top (the bitmap is inverted vertically
					BYTE *p32 = (BYTE *)bm32.bmBits + (bm32.bmHeight - 1) * bm32.bmWidthBytes;
					for (int y = 0; y < bm.bmHeight; y++)
					{
						// Scan each bitmap pixel from left to righ
						for (int x = 0; x < bm.bmWidth; x++)
						{
							// Search for a continuous range of "non transparent pixels"
							int x0 = x;
							LONG *p = (LONG *)p32 + x;
							while (x < bm.bmWidth)
							{
								BYTE b = GetRValue(*p);
								if (b >= lr && b <= hr)
								{
									b = GetGValue(*p);
									if (b >= lg && b <= hg)
									{
										b = GetBValue(*p);
										if (b >= lb && b <= hb)
											// This pixel is "transparent"
											break;
									}
								}
								p++;
								x++;
							}

							if (x > x0)
							{
								// Add the pixels (x0, y) to (x, y+1) as a new rectangle in the regio
								if (pData->rdh.nCount >= maxRects)
								{
									GlobalUnlock(hData);
									maxRects += ALLOC_UNIT;
									hData = GlobalReAlloc(hData, sizeof(RGNDATAHEADER) + (sizeof(RECT) * maxRects), GMEM_MOVEABLE);
									pData = (RGNDATA *)GlobalLock(hData);
								
								}
								RECT *pr = (RECT *)&pData->Buffer;
								SetRect(&pr[pData->rdh.nCount], x0, y, x, y+1);
								if (x0 < pData->rdh.rcBound.left)
									pData->rdh.rcBound.left = x0;
								if (y < pData->rdh.rcBound.top)
									pData->rdh.rcBound.top = y;
								if (x > pData->rdh.rcBound.right)
									pData->rdh.rcBound.right = x;
								if (y+1 > pData->rdh.rcBound.bottom)
									pData->rdh.rcBound.bottom = y+1;
								pData->rdh.nCount++;

								/*
								// On Windows98, ExtCreateRegion() may fail if the number of rectangles is to
								// large (ie: > 4000). Therefore, we have to create the region by multiple steps
								if (pData->rdh.nCount == 2000)
								{
									HRGN h = ExtCreateRegion(NULL, sizeof(RGNDATAHEADER) + (sizeof(RECT) * maxRects), pData);
									
									// Free the data
									GlobalFree(hData);

									if (hRgn)
									{
										CombineRgn(hRgn, hRgn, h, RGN_OR);
										DeleteObject(h);
									}
									else
										hRgn = h;
									pData->rdh.nCount = 0;
									SetRect(&pData->rdh.rcBound, MAXLONG, MAXLONG, 0, 0);
								}
								*/
							}
						}

						// Go to next row (remember, the bitmap is inverted vertically
						p32 -= bm32.bmWidthBytes;
					}

					// Create or extend the region with the remaining rectangle
					HRGN h = ExtCreateRegion(NULL, sizeof(RGNDATAHEADER) + (sizeof(RECT) * maxRects), pData);

					if (hRgn)
					{
						CombineRgn(hRgn, hRgn, h, RGN_OR);
						DeleteObject(h);
					}
					else
						hRgn = h;

					// Clean u
					SelectObject(hDC, holdBmp);
					DeleteDC(hDC);
				}

				DeleteObject(SelectObject(hMemDC, holdBmp));
			}

			DeleteDC(hMemDC);
		}
	}

	return hRgn;
}
	
HitRegion* HitRegion::Clone()
{
	HitRegion* temp = new HitRegion();

	temp->m_HitRegion = CreateRectRgn(0, 0, 10, 10); // create dummy region
	CombineRgn(temp->m_HitRegion, m_HitRegion, 0, RGN_COPY);

	return temp;
}
	
void HitRegion::Move(int x, int y)
{
	OffsetRgn(m_HitRegion, x, y);
}
	
RECT HitRegion::GetDimension()
{
	RECT boundingbox;
	GetRgnBox(m_HitRegion, &boundingbox);

	return boundingbox;
}

HRGN HitRegion::GetHandle()
{
	return m_HitRegion;
}

bool HitRegion::HitTest(HitRegion* regPtr)
{
	HRGN temp = CreateRectRgn(0, 0, 10, 10);			// dummy region
	bool result = (CombineRgn(temp, m_HitRegion, regPtr->m_HitRegion, RGN_AND) != NULLREGION);

	DeleteObject(temp);
	return result;
}
	
bool HitRegion::HitTest(int x, int y)
{
	return PtInRegion(m_HitRegion, x, y)?true:false;
}
	
RECT HitRegion::CollisionTest(HitRegion* regPtr)
{
	// initalize dummy region
	HRGN temp = CreateRectRgn(0, 0, 10, 10);			
	int overlap = CombineRgn(temp, m_HitRegion, regPtr->m_HitRegion, RGN_AND);
	
	RECT result;
	GetRgnBox(temp, &result);

	DeleteObject(temp);
	
	return result;
}