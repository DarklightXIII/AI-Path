//-----------------------------------------------------------------
// Game Engine WinMain Function
// C++ Source - GameWinMain.cpp - version 2010 v2_07
// Copyright Kevin Hoefman - kevin.hoefman@howest.be
// http://www.digitalartsandentertainment.be/
//-----------------------------------------------------------------

//-----------------------------------------------------------------
// Include Files
//-----------------------------------------------------------------
#include "GameWinMain.h"
#include "GameEngine.h"

#include "AIchallenge.h"	

//-----------------------------------------------------------------
// Defines
//-----------------------------------------------------------------

#define GAME_ENGINE (GameEngine::GetSingleton())

//-----------------------------------------------------------------
// Windows Functions
//-----------------------------------------------------------------
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR szCmdLine, int iCmdShow)
{
	if (GAME_ENGINE == NULL) return FALSE; // create the game engine and exit if it fails
	
	GAME_ENGINE->SetGame(new AIchallenge());	// any class that implements AbstractGame
	
	return GAME_ENGINE->Run(hInstance, iCmdShow); // run the game engine and return the result
}
