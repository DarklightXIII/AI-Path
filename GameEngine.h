//-----------------------------------------------------------------
// Game Engine Object
// C++ Header - GameEngine.h - version 2010 v2_07
// Copyright Kevin Hoefman - kevin.hoefman@howest.be
// Thanks to Bart Uyttenhove - bart.uyttenhove@howest.be
//		for several changes & contributions
// http://www.digitalartsandentertainment.be/
//-----------------------------------------------------------------
// Changes since v2_06:
// - Removed overloaded Draw methods that had an extra HDC parameter
// - Changed game loop to use timeGetTime() instead of GetTickCount()
// - Changed HitRegion::CollisionTest() to return a RECT instead of a point
// - 25/09/10: Removed parameters from AbstractGame::GameActivate() and AbstractGame::GameDeactivate()

#pragma once

//-----------------------------------------------------------------
// Include Files
//-----------------------------------------------------------------
#define _WIN32_WINNT 0x0601 // Windows 7
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdlib.h>
#include <tchar.h>
#include <Mmsystem.h>	// winmm.lib header, used for playing sound
#undef MessageBox

#include "AbstractGame.h" // needed to use abstract class as basis for all games

#include <vector>			// using std::vector for tab control logic#include <vector>
#include <queue>
#include <algorithm>
using namespace std;

#include <sstream>

//-----------------------------------------------------------------
// Pragma Library includes
//-----------------------------------------------------------------
#pragma comment(lib, "msimg32.lib")		// used for transparency
#pragma comment(lib, "winmm.lib")		// used for sound

//-----------------------------------------------------------------
// GameEngine Defines
//-----------------------------------------------------------------
#define KEYBCHECKRATE 60

//-----------------------------------------------------------------
// GameEngine Forward Declarations
//-----------------------------------------------------------------
class Bitmap;
class SoundWave;
class Midi;
class String;
class HitRegion;

//-----------------------------------------------------------------
// GameEngine Class
//-----------------------------------------------------------------
class GameEngine
{
private:
	// singleton implementation : private constructor + static pointer to game engine
	GameEngine();
	static GameEngine*  m_GameEnginePtr;

public:
	// Destructor
	virtual ~GameEngine();

	// Static methods
	static GameEngine*  GetSingleton();

	// General Methods
	void		SetGame(AbstractGame* gamePtr);
	bool		Run(HINSTANCE hInstance, int iCmdShow);
	bool        ClassRegister(int iCmdShow);
	bool		SetGameValues(String const& titleRef, WORD wIcon, WORD wSmallIcon, int iWidth, int iHeight);
	bool		SetWindowRegion(HitRegion* regionPtr); // new 2_03
	bool		HasWindowRegion(); // new 2_03
	bool		GoFullscreen();		// new 2_03
	bool		GoWindowedMode();		// new 2_03
	bool		IsFullscreen();		// new 2_03
	void		ShowMousePointer(bool value);	// new 2_03

	LRESULT     HandleEvent(HWND hWindow, UINT msg, WPARAM wParam, LPARAM lParam);
	bool		IsKeyDown(int vKey);
	void		QuitGame(void);
	void		MessageBox(double value);
	void		MessageBox(int value);
	void		MessageBox(size_t value);
	void		MessageBox(String const& textRef);
	void		RunGameLoop(bool value);
	void		TabNext(HWND ChildWindow);
	void		TabPrevious(HWND ChildWindow);

	// Draw Methods
	bool		DrawLine(int x1, int y1, int x2, int y2);
	bool		DrawPolygon(const POINT ptsArr[], int count);
	bool		DrawPolygon(const POINT ptsArr[], int count, bool close);
	bool		FillPolygon(const POINT ptsArr[], int count);
	bool		FillPolygon(const POINT ptsArr[], int count, bool close);
	bool		DrawRect(int x, int y, int width, int height);
	bool		FillRect(int x, int y, int width, int height);
	bool		DrawRoundRect(int x, int y, int width, int height, int radius);
	bool		FillRoundRect(int x, int y, int width, int height, int radius);
	bool		DrawOval(int x, int y, int width, int height);
	bool		FillOval(int x, int y, int width, int height);
	bool		DrawArc(int x, int y, int width, int height, int startDegree, int angle);
	bool		FillArc(int x, int y, int width, int height, int startDegree, int angle);
	int			DrawString(String const& textRef, int x, int y);
	int			DrawString(String const& textRef, int x, int y, int width, int height);
	bool		DrawBitmap(Bitmap* bitmapPtr, int x, int y);
	bool		DrawBitmap(Bitmap* bitmapPtr, int x, int y, RECT rect);
	bool		DrawSolidBackground(COLORREF color);
	void		SetColor(COLORREF color) { m_ColDraw = color; }
	COLORREF	GetDrawColor() { return m_ColDraw; }
	void		SetFont(String const& fontNameRef, bool bold, bool italic, bool underline, int size);
	bool		Repaint();

	// Accessor Methods
	HINSTANCE	GetInstance() { return m_hInstance; }
	HWND		GetWindow() { return m_hWindow; }
	String&		GetTitle() { return *m_TitlePtr; }
	WORD		GetIcon() { return m_wIcon; }
	WORD		GetSmallIcon() { return m_wSmallIcon; }
	int			GetWidth() { return m_iWidth; }
	int			GetHeight() { return m_iHeight; }
	int			GetFrameDelay() { return m_iFrameDelay; }
	bool		GetSleep() { return m_bSleep?true:false; }
	POINT		GetLocation();

	// Public Mutator Methods	
	void		SetTitle(String const& titleRef); // SetTitle automatically sets the window class name to the same name as the title - easier for students 
	void		SetIcon(WORD wIcon) { m_wIcon = wIcon; }
	void		SetSmallIcon(WORD wSmallIcon) { m_wSmallIcon = wSmallIcon; }
	void		SetWidth(int iWidth) { m_iWidth = iWidth; }
	void		SetHeight(int iHeight) { m_iHeight = iHeight; }
	void		SetFrameRate(double iFrameRate) { m_iFrameDelay = (int) (1000 / iFrameRate); }
	void		SetSleep(bool bSleep) { m_bSleep = bSleep; }
	void		SetKeyList(String const& keyListRef);
	void		SetPaintDoublebuffered() { m_PaintDoublebuffered = true; }	
	void		SetLocation(int x, int y);

	// Keyboard monitoring thread method
	virtual DWORD		KeybThreadProc();
	
private:
	// Private Mutator Methods	
	void		SetInstance(HINSTANCE hInstance) { m_hInstance = hInstance; };
	void		SetWindow(HWND hWindow) { m_hWindow = hWindow; };

	// Private Draw Methods
	void FormPolygon(const POINT ptsArr[], int count, bool close);
	POINT AngleToPoint(int x, int y, int width, int height, int angle);

	// Member Variables
	HINSTANCE           m_hInstance;
	HWND                m_hWindow;
	String*             m_TitlePtr;
	WORD                m_wIcon, m_wSmallIcon;
	int                 m_iWidth, m_iHeight;
	int                 m_iFrameDelay;
	bool                m_bSleep;
	bool				m_bRunGameLoop;
	HANDLE				m_hKeybThread;
	DWORD				m_dKeybThreadID;
	bool				m_bKeybRunning;
	TCHAR*				m_KeyListPtr;
	unsigned int		m_KeybMonitor;
	AbstractGame*		m_GamePtr;
	bool				m_PaintDoublebuffered;
	bool				m_Fullscreen;

	// Draw assistance variables
	HDC m_HdcDraw;
	RECT m_RectDraw;
	bool m_IsPainting, m_IsDoublebuffering;
	COLORREF m_ColDraw;
	HFONT m_FontDraw;

	// Fullscreen assistance variable
	POINT m_OldLoc;

	// Window Region assistance variable
	HitRegion* m_WindowRegionPtr;
		
	// -------------------------
	// Disabling default copy constructor and default assignment operator.
	// If you get a linker error from one of these functions, your class is internally trying to use them. This is
	// an error in your class, the declarations are deliberately made without implementation because they should never be used.
	// -------------------------
	GameEngine(const GameEngine& geRef);
	GameEngine& operator=(const GameEngine& geRef);
};

//-----------------------------------------------------------------
// String Class
//-----------------------------------------------------------------
class String
{
public: 
	// -------------------------
	// Constructors & Destructor
	// -------------------------	
	String(wchar_t const* wideTextPtr = L"");
	String(char const* singleTextPtr);
	String(String const& sRef);
	String(wchar_t character);
	String(char character);

	virtual ~String();

	// -------------------------
	// General String Methods
	// -------------------------	
	TCHAR CharAt(int index) const;
	String Replace(TCHAR oldChar, TCHAR newChar) const;
	String SubString(int index) const;
	String SubString(int index, int length) const;
	String ToLowerCase() const;
	String ToUpperCase() const;
	String Trim() const;
	int IndexOf(TCHAR character) const;
	int LastIndexOf(TCHAR character) const;
	bool StartsWith(String const& sRef) const;
	bool EndsWith(String const& sRef) const;
	int GetLength() const;
	bool Equals(String const& sRef) const;

	// -------------------------
	// Conversion Methods
	// -------------------------	
	TCHAR* ToTChar() const;
	int ToInteger() const;
	double ToDouble() const;

	// ----------------------------------------
	// Overloaded operators: = , +=, +, and ==
	// ----------------------------------------
	String& operator=(String const& sRef);

	String& operator+=(String const& sRef);
	String& operator+=(wchar_t* wideTextPtr);
	String& operator+=(char* singleTextPtr);
	String& operator+=(int number);
	String& operator+=(size_t number);
	String& operator+=(double number);
	String& operator+=(wchar_t character);
	String& operator+=(char character);

	String operator+(String const& sRef);
	String operator+(wchar_t* wideTextPtr);
	String operator+(char* singleTextPtr);
	String operator+(int number);
	String operator+(size_t number);
	String operator+(double number);
	String operator+(wchar_t character);
	String operator+(char character);

	bool operator==(String const& sRef);
	bool operator== (String const& sRef) const;

private:
	// -------------------------
	// Datamembers
	// -------------------------
	TCHAR* m_TextPtr;
	int m_Length;
};

//------------------------------------------------------------------------------------------------
// Callable Interface
//
// Interface implementation for classes that can be called by "caller" objects
//------------------------------------------------------------------------------------------------
class Caller;	// forward declaration

class Callable
{
public:
	virtual ~Callable() {}						// virtual destructor for polymorphism
	virtual void CallAction(Caller* callerPtr) = 0;
};

//------------------------------------------------------------------------------------------------
// Caller Base Class
//
// Base Clase implementation for up- and downcasting of "caller" objects: TextBox, Button, Timer, Audio and Video
//------------------------------------------------------------------------------------------------
class Caller
{
public:
	virtual ~Caller() {}				// do not delete the targets!

	static const int TextBox = 0;
	static const int Button = 1;
	static const int Timer = 2;
	static const int Audio = 3;
	static const int Video = 4;

	virtual int GetType() = 0;

	virtual bool AddActionListener(Callable* targetPtr);		// public interface method, call is passed on to private method
	virtual bool RemoveActionListener(Callable* targetPtr);	// public interface method, call is passed on to private method

protected:
	Caller() {}								// constructor only for derived classes
	vector<Callable*> m_TargetList;

	virtual bool CallListeners();			// placing the event code in a separate method instead of directly in the windows messaging
											// function allows inheriting classes to override the event code. 

private:
	bool AddListenerObject(Callable* targetPtr);
	bool RemoveListenerObject(Callable* targetPtr);

	// -------------------------
	// Disabling default copy constructor and default assignment operator.
	// If you get a linker error from one of these functions, your class is internally trying to use them. This is
	// an error in your class, the declarations are deliberately made without implementation because they should never be used.
	// -------------------------
	Caller(const Caller& cRef);
	Caller& operator=(const Caller& cRef);
};

//--------------------------------------------------------------------------
// Timer Class
//
// This timer is a queue timer, it will only work on Windows 2000 and higher
//--------------------------------------------------------------------------

class Timer : public Caller
{
public:
	Timer(int msec, Callable* targetPtr); // constructor automatically adds 2nd parameter to the list of listener objects

	virtual ~Timer();

	int GetType() {return Caller::Timer;}
	void Start();
	void Stop();
	bool IsRunning();
	void SetDelay(int msec);
	int GetDelay();

private:	
	// -------------------------
	// Datamembers
	// -------------------------
	HANDLE m_TimerHandle;
	bool m_IsRunning;
	int m_Delay;

	// -------------------------
	// Handler functions
	// -------------------------	
	static void CALLBACK TimerProcStatic(void* lpParameter, BOOLEAN TimerOrWaitFired); // proc will call CallListeners()
		
	// -------------------------
	// Disabling default constructor, default copy constructor and default assignment operator.
	// If you get a linker error from one of these functions, your class is internally trying to use them. This is
	// an error in your class, these declarations are deliberately made without implementation because they should never be used.
	// -------------------------
	Timer();
	Timer(const Timer& tRef);
	Timer& operator=(const Timer& tRef);
};

//-----------------------------------------------------------------
// TextBox Class
//-----------------------------------------------------------------

class TextBox : public Caller
{
public:
	TextBox(String const& textRef);
	TextBox();

	virtual ~TextBox();

	int GetType() {return Caller::TextBox;}
	void SetBounds(int x, int y, int width, int height);
	String GetText();
	void SetText(String const& textRef);
	void SetFont(String const& fontNameRef, bool bold, bool italic, bool underline, int size);
	void SetBackcolor( COLORREF color );
	void SetForecolor( COLORREF color );
	COLORREF GetForecolor();
	COLORREF GetBackcolor();
	HBRUSH GetBackcolorBrush();
	RECT GetRect();
	void SetEnabled(bool bEnable);
	void Update(void);
	void Show();
	void Hide();

private:
	// -------------------------
	// Datamembers
	// -------------------------
	int m_x, m_y;
	HWND m_hWndEdit;
	WNDPROC m_procOldEdit;
	COLORREF m_BgColor, m_ForeColor;
	HBRUSH m_BgColorBrush;
	HFONT m_Font, m_OldFont;

	// -------------------------
	// Handler functions
	// -------------------------	
	static LRESULT CALLBACK EditProcStatic(HWND hWindow, UINT msg, WPARAM wParam, LPARAM lParam);
	LRESULT EditProc(HWND hWindow, UINT msg, WPARAM wParam, LPARAM lParam);
		
	// -------------------------
	// Disabling default copy constructor and default assignment operator.
	// If you get a linker error from one of these functions, your class is internally trying to use them. This is
	// an error in your class, these declarations are deliberately made without implementation because they should never be used.
	// -------------------------
	TextBox(const TextBox& tbRef);
	TextBox& operator=(const TextBox& tbRef);
};

//-----------------------------------------------------------------
// Button Class
//-----------------------------------------------------------------
class Button : public Caller
{
public:
	Button(String const& textRef);
	Button();

	virtual ~Button();

	int GetType() {return Caller::Button;}
	void SetBounds(int x, int y, int width, int height);
	String GetText();
	void SetText(String const& textRef);
	void SetFont(String const& fontNameRef, bool bold, bool italic, bool underline, int size);
	RECT GetRect();
	void SetEnabled(bool bEnable);
	void Update(void);
	void Show();
	void Hide();

private:
	// -------------------------
	// Datamembers
	// -------------------------
	int m_x, m_y;
	HWND m_hWndButton;
	WNDPROC m_procOldButton;
	bool m_Armed;
	COLORREF m_BgColor, m_ForeColor;
	HFONT m_Font, m_OldFont;

	// -------------------------
	// Handler functions
	// -------------------------	
	static LRESULT CALLBACK ButtonProcStatic(HWND hWindow, UINT msg, WPARAM wParam, LPARAM lParam);
	LRESULT ButtonProc(HWND hWindow, UINT msg, WPARAM wParam, LPARAM lParam);
		
	// -------------------------
	// Disabling default copy constructor and default assignment operator.
	// If you get a linker error from one of these functions, your class is internally trying to use them. This is
	// an error in your class, these declarations are deliberately made without implementation because they should never be used.
	// -------------------------
	Button(const Button& bRef);
	Button& operator=(const Button& bRef);
};

//-----------------------------------------------------------------
// Audio Class
//-----------------------------------------------------------------
class Audio : public Caller
{
public:
	Audio(String const& nameRef);
	Audio(int IDAudio, String const& typeRef);

	virtual ~Audio();

	String& GetName();
	String& GetAlias();
	int GetDuration();
	bool IsPlaying();
	bool IsPaused();
	void SwitchPlayingOff();				// internal use only, don't use this unless you know what you're doing
	void SetRepeat(bool repeat);
	bool GetRepeat();
	bool Exists();
	int GetVolume();
	int GetType();

	// these methods are called to instruct the object. The methods that perform the actual sendstrings are private.
	void Play(int msecStart = 0, int msecStop = -1);
	void Pause();
	void Stop();
	void SetVolume(int volume);

	// commands are queued and sent through a Tick() which should be called by the main thread (mcisendstring isn't thread safe) 
	// has the additional benefit of creating a delay between mcisendstring commands
	void Tick();

private:	
	// -------------------------
	// Datamembers
	// -------------------------
	static int m_nr;
	String m_FileName;
	String m_Alias;
	bool m_Playing, m_Paused;
	bool m_MustRepeat;
	HWND m_hWnd;
	int m_Duration;
	int m_Volume;

	// -------------------------
	// Member functions
	// -------------------------	
	
	void Create(String const& nameRef);
	void Extract(WORD id, String sType, String sFilename);

	// private mcisendstring command methods and command queue datamember
	queue<String> m_CommandQueue;
	void QueuePlayCommand(int msecStart);
	void QueuePlayCommand(int msecStart, int msecStop);
	void QueuePauseCommand();
	void QueueResumeCommand();
	void QueueStopCommand();
	void QueueVolumeCommand(int volume);		// add a m_Volume datamember? What volume does the video start on by default?
	void QueuePositionCommand(int x, int y);
	void QueueCommand(String const& commandRef);
	void SendMCICommand(String const& commandRef);
			
	// -------------------------
	// Handler functions
	// -------------------------	
	static LRESULT CALLBACK AudioProcStatic(HWND hWindow, UINT msg, WPARAM wParam, LPARAM lParam);
	
	// -------------------------
	// Disabling default constructor, default copy constructor and default assignment operator.
	// If you get a linker error from one of these functions, your class is internally trying to use them. This is
	// an error in your class, these declarations are deliberately made without implementation because they should never be used.
	// -------------------------
	Audio();
	Audio(const Audio& aRef);
	Audio& operator=(const Audio& aRef);
};

//-----------------------------------------------------------------
// Bitmap Class
//-----------------------------------------------------------------

class Bitmap
{
public:
	Bitmap(String const& nameRef, bool createAlphaChannel = true);						
	Bitmap(int IDBitmap, String const& sTypeRef, bool createAlphaChannel = true);

	virtual ~Bitmap();

	bool Exists();
	HBITMAP GetHandle();
	int GetWidth();
	int GetHeight();
	void SetTransparencyColor(COLORREF color);
	COLORREF GetTransparencyColor();
	void SetOpacity(int);
	int GetOpacity();
	bool IsTarga();
	bool HasAlphaChannel();
	
private:	
	// -------------------------
	// Datamembers
	// -------------------------
	static int m_nr;
	HBITMAP m_Handle;
	COLORREF m_TransparencyKey;
	int m_Opacity;
	bool m_IsTarga;
	unsigned char* m_PixelsPtr; 
	bool m_Exists;
	bool m_HasAlphaChannel;

	// -------------------------
	// Methods
	// -------------------------
	void LoadBitInfo(); 
	void Extract(WORD id, String sType, String sFilename);
		
	// -------------------------
	// Disabling default constructor, default copy constructor and default assignment operator.
	// If you get a linker error from one of these functions, your class is internally trying to use them. This is
	// an error in your class, these declarations are deliberately made without implementation because they should never be used.
	// -------------------------
	Bitmap();
	Bitmap(const Bitmap& bRef);
	Bitmap& operator=(const Bitmap& bRef);
};

//-----------------------------------------------------------------
// TGA Loader Class - 16/11/04 Codehead - original name TGAImg
//-----------------------------------------------------------------
class TargaLoader
{
public:
	TargaLoader();
	~TargaLoader();
	int Load(TCHAR* szFilenamePtr);
	int GetBPP();
	int GetWidth();
	int GetHeight();
	unsigned char* GetImg();       // Return a pointer to image data
	unsigned char* GetPalette();   // Return a pointer to VGA palette
 
private:
	short int iWidth,iHeight,iBPP;
	unsigned long lImageSize;
	char bEnc;
	unsigned char *pImage, *pPalette, *pData;
   
	// Internal workers
	int ReadHeader();
	int LoadRawData();
	int LoadTgaRLEData();
	int LoadTgaPalette();
	void BGRtoRGB();
	void FlipImg();
};

//-----------------------------------------------------------------
// Extra OutputDebugString functions
//-----------------------------------------------------------------
void OutputDebugString(String const& textRef);

//-----------------------------------------------------------------
// Windows Procedure Declarations
//-----------------------------------------------------------------
DWORD WINAPI		KeybThreadProc (GameEngine* gamePtr);
LRESULT CALLBACK	WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);


//-----------------------------------------------------------------
// HitRegion Class								
//-----------------------------------------------------------------
class HitRegion
{
public: 
	//---------------------------
	// Constructor(s)
	//---------------------------
	HitRegion();						// Default constructor. Geef de waarden van de nieuwe region op via de Create( ) methode.

	//---------------------------
	// Destructor
	//---------------------------
	virtual ~HitRegion();			

	//---------------------------
	// General Methods
	//---------------------------
	bool Create(int type, int x, int y, int width, int height);	// Gebruik deze create om een rechthoekige of elliptische hitregion te vormen

	bool Create(int type, POINT* pointsArr, int numberOfPoints);	// Gebruik deze create om een polygonregion te vormen
	
	bool Create(int type, Bitmap* bmpPtr, COLORREF cTransparent = RGB(255, 0, 255), COLORREF cTolerance = 0); // Gebruik deze create om een region uit een bitmap te maken

	HitRegion* Clone();											// Maakt een nieuw HitRegion object met dezelfde data als dit hitregion object, en returnt een pointerwaarde ernaartoe

	bool HitTest(HitRegion* regPtr);			// Returnt true als de regions overlappen, false als ze niet overlappen
	bool HitTest(int x, int y);				// Returnt true als het punt dat overeenkomt met de opgegeven x- en y-waarden in de region ligt, false indien niet.

	RECT CollisionTest(HitRegion* regPtr);	// Returnt de rechthoek gevormd door de overlappende regio van de twee hitregions. 
											// Als er geen overlapping is, is de rechthoek = {0, 0, 0, 0}.											
	
	RECT GetDimension();					// Geeft de positie + breedte en hoogte terug van de kleinste rechthoek die deze region omhult (bij rechthoekige regions: de region zelf) 			

	HRGN GetHandle();						// Geeft de handle van de region weer (Win32 stuff)

	void Move(int x, int y);				// Verplaatst de volledige region volgens de opgegeven verplaatsing over x en y 

	static const int Ellipse = 0;			// Klasse-constanten om te kunnen opgeven welke soort region er moet gemaakt worden (analoog aan enum)
	static const int Rectangle = 1;
	static const int Polygon = 2;
	static const int FromBitmap = 3;

private:
	//---------------------------
	// Datamembers
	//---------------------------
	HRGN m_HitRegion;						// De region data wordt bijgehouden door middel van een Win32 "region" resource (niet kennen 1e semester).

	//---------------------------
	// Private methods
	//---------------------------
	HRGN BitmapToRegion(HBITMAP hBmp, COLORREF cTransparentColor, COLORREF cTolerance);

	// -------------------------
	// Disabling default copy constructor and default assignment operator.
	// If you get a linker error from one of these functions, your class is internally trying to use them. This is
	// an error in your class, these declarations are deliberately made without implementation because they should never be used.
	// -------------------------
	HitRegion(const HitRegion& hrRef);				
	HitRegion& operator=(const HitRegion& hrRef);	
};

