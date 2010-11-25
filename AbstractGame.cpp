//-----------------------------------------------------------------
// AbstractGame Object
// C++ Source - AbstractGame.cpp - version 2010 v2_07
// Copyright Kevin Hoefman - kevin.hoefman@howest.be
// http://www.digitalartsandentertainment.be/
//-----------------------------------------------------------------

//-----------------------------------------------------------------
// Include Files
//-----------------------------------------------------------------
#include "Resource.h"	// include file om de resources te kunnen gebruiken
#include "GameEngine.h" // include file om de game engine te kunnen aanspreken
#include "AbstractGame.h"

//-----------------------------------------------------------------
// Defines
//-----------------------------------------------------------------

#define GAME_ENGINE (GameEngine::GetSingleton())

//-----------------------------------------------------------------
// AbstractGame methods
//-----------------------------------------------------------------

void AbstractGame::GameInitialize(HINSTANCE hInstance)
{
	// Stel de verplichte waarden in
	GAME_ENGINE->SetTitle("Game Engine version 2_07");
	GAME_ENGINE->SetIcon(IDI_BIG);
	GAME_ENGINE->SetSmallIcon(IDI_SMALL);
	//GAME_ENGINE->RunGameLoop(true);

	// Stel de optionele waarden in
	GAME_ENGINE->SetWidth(640);
	GAME_ENGINE->SetHeight(480);
	GAME_ENGINE->SetFrameRate(50);
}