//-----------------------------------------------------------------
// Game File
// C++ Header - AIchallenge.h - version 2010 v2_07
// Copyright Kevin Hoefman - kevin.hoefman@howest.be
// http://www.digitalartsandentertainment.be/
//-----------------------------------------------------------------

#pragma once

//-----------------------------------------------------------------
// Include Files
//-----------------------------------------------------------------

#include "Resource.h"	
#include "GameEngine.h"
#include "AbstractGame.h"


//-----------------------------------------------------------------
// Enums
//-----------------------------------------------------------------

enum DIRECTION
{
	left,
	up,
	right,
	down
};

//-----------------------------------------------------------------
// Structs
//-----------------------------------------------------------------

struct AI_PLAYER
{
	int xPos, yPos;
	COLORREF color;
	//DIRECTION direction;
	int direction;
};

struct GRID
{
	//width and height are in the number of cells
	//the size of the cells is in pixels
	int width;
	int height;
	int cellSize;
};

//-----------------------------------------------------------------
// AIchallenge Class																
//-----------------------------------------------------------------
class AIchallenge : public AbstractGame, public Callable
{
public:				
	//---------------------------
	// Constructor(s)
	//---------------------------
	AIchallenge();

	//---------------------------
	// Destructor
	//---------------------------
	virtual ~AIchallenge();

	//---------------------------
	// General Methods
	//---------------------------

	void GameInitialize(HINSTANCE hInstance);
	void GameStart();				
	void GameEnd();
	void GameActivate();
	void GameDeactivate();
	void MouseButtonAction(bool isLeft, bool isDown, int x, int y, WPARAM wParam);
	void MouseMove(int x, int y, WPARAM wParam);
	void CheckKeyboard();
	void KeyPressed(TCHAR cKey);
	void GamePaint(RECT rect);
	void GameCycle(RECT rect);
	void DrawAIplayer(AI_PLAYER player);
	void DrawRigidBodies();
	AI_PLAYER MoveAIplayer(AI_PLAYER player);

	void CallAction(Caller* callerPtr);

private:
	// -------------------------
	// Datamembers
	// -------------------------
	int m_gridSize;
	AI_PLAYER m_default;
	//GRID m_isRigidCell;
	bool **m_isRigidCell;
	// -------------------------
	// Disabling default copy constructor and default assignment operator.
	// If you get a linker error from one of these functions, your class is internally trying to use them. This is
	// an error in your class, these declarations are deliberately made without implementation because they should never be used.
	// -------------------------
	AIchallenge(const AIchallenge& tRef);
	AIchallenge& operator=(const AIchallenge& tRef);
};
