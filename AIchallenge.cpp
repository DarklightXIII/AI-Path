//-----------------------------------------------------------------
// GAME ENGINE
// C++ Source - AIchallenge.cpp - version 2010 v2_07
// Copyright Kevin Hoefman - kevin.hoefman@howest.be
// http://www.digitalartsandentertainment.be/
//
//	AI Algorythm and game platform - (c)2010
//	by Cedric Van der Kelen - Cedric.Van.der.Kelen@howest.be
//-----------------------------------------------------------------

//-----------------------------------------------------------------
// Include Files
//-----------------------------------------------------------------
#include "AIchallenge.h"																				

//-----------------------------------------------------------------
// Defines
//-----------------------------------------------------------------
#define GAME_ENGINE (GameEngine::GetSingleton())

//-----------------------------------------------------------------
// AIchallenge methods																				
//-----------------------------------------------------------------
int _fpst;

AIchallenge::AIchallenge():m_gridSize(40),
							m_default(),
							m_isRigidCell(),
							m_filler(),
							m_berserker()
{

}

AIchallenge::~AIchallenge()																						
{
	for (int i = 0; i < 48; ++i)
	{
		delete [] m_isRigidCell[i];
	}
	delete [] m_isRigidCell;
}

void AIchallenge::GameInitialize(HINSTANCE hInstance)			
{
	AbstractGame::GameInitialize(hInstance);
	GAME_ENGINE->SetTitle("AIchallenge");					
	GAME_ENGINE->RunGameLoop(true);

	// Stel de optionele waarden in
	GAME_ENGINE->SetWidth(800);
	GAME_ENGINE->SetHeight(800);
    GAME_ENGINE->SetFrameRate(20);
}

void AIchallenge::GameStart()
{
	//initialising the AI's
	m_default.name = "default test AI";
	m_default.xPos = GAME_ENGINE->GetWidth() / 2 / m_gridSize;
	m_default.yPos = GAME_ENGINE->GetHeight() / 2 / m_gridSize;
	m_default.playerColor = RGB(255,150,150);

	m_berserker.name = "berserker (random AI)";
	m_berserker.xPos = 2;
	m_berserker.yPos = GAME_ENGINE->GetHeight() / 2 / m_gridSize;
	m_berserker.playerColor = RGB(255,0,0);
	m_berserker.fillColor = RGB(255,150,150);

	m_filler.name = "filler (fill AI)";
	m_filler.xPos = GAME_ENGINE->GetWidth() / m_gridSize - 2;
	m_filler.yPos = GAME_ENGINE->GetHeight() / 2 / m_gridSize;
	m_filler.playerColor = RGB(0,0,255);
	m_filler.fillColor = RGB(150,150,255);
	
	//Rigid Cell List Allocating
	m_isRigidCell = new bool*[GAME_ENGINE->GetWidth() / m_gridSize];
	for (int i = 0; i < GAME_ENGINE->GetWidth() / m_gridSize; ++i)
	{
		m_isRigidCell[i] = new bool[GAME_ENGINE->GetHeight() / m_gridSize];
	}

	//rigid cell ini
	for(int x = 0;x < (GAME_ENGINE->GetWidth() / m_gridSize);++x)
	{
		for(int y = 0; y < (GAME_ENGINE->GetHeight() / m_gridSize);++y)
		{
			m_isRigidCell[x][y] = false;
			m_isRigidCell[0][y] = true;
			m_isRigidCell[GAME_ENGINE->GetWidth() / m_gridSize - 1][y] = true;
		}
		m_isRigidCell[x][0] = true;
		m_isRigidCell[x][GAME_ENGINE->GetHeight() / m_gridSize - 1] = true;
	}
}
void AIchallenge::GameEnd()
{
}
void AIchallenge::GameActivate()
{
}
void AIchallenge::GameDeactivate()
{
}
void AIchallenge::MouseButtonAction(bool isLeft, bool isDown, int x, int y, WPARAM wParam)
{	
}
void AIchallenge::MouseMove(int x, int y, WPARAM wParam)
{	
}
void AIchallenge::CheckKeyboard()
{	
}
void AIchallenge::KeyPressed(TCHAR cKey)
{
}
void AIchallenge::GamePaint(RECT rect)
{
}
void AIchallenge::GameCycle(RECT rect)
{
	GAME_ENGINE->DrawSolidBackground(RGB(255,255,255));

	//draw Grid
	/*GAME_ENGINE->SetColor(RGB(0,0,0));
	for(int i = 0; i < GAME_ENGINE->GetWidth() / 10;i++)
	{
		GAME_ENGINE->DrawLine(i*m_gridSize,0,i*m_gridSize,GAME_ENGINE->GetHeight());
	}
	for(int i = 0; i < GAME_ENGINE->GetHeight() / 10;i++)
	{
		GAME_ENGINE->DrawLine(0,i*m_gridSize,GAME_ENGINE->GetWidth(),i*m_gridSize);
	}*/



	//Move the AI's
	if(_fpst % 2== 0)
	{
		//m_default = MoveAIplayer(m_default);
		m_berserker = MoveAIplayer(m_berserker);
		m_filler = MoveAIplayer(m_filler,0);
	}

	//Draw the rigid cells
	DrawRigidBodies();

	//Draw the AI
	//DrawAIplayer(m_default);
	DrawAIplayer(m_berserker);
	DrawAIplayer(m_filler);

	_fpst++;
}

void AIchallenge::DrawAIplayer(AI_PLAYER player)
{
	GAME_ENGINE->SetColor(player.playerColor);
	GAME_ENGINE->FillRect(player.xPos * m_gridSize + 1, player.yPos * m_gridSize + 1,m_gridSize -1,m_gridSize-1);
}

void AIchallenge::DrawRigidBodies()
{
	GAME_ENGINE->SetColor(RGB(120,120,120));
	for(int x = 0;x < GAME_ENGINE->GetWidth() / m_gridSize;++x)
	{
		for(int y = 0; y < (GAME_ENGINE->GetHeight() / m_gridSize);++y)
		{
			if(m_isRigidCell[x][y])
			{
				GAME_ENGINE->FillRect(x * m_gridSize + 1, y * m_gridSize + 1, m_gridSize - 1, m_gridSize - 1);
			}
		}
	}
}

AI_PLAYER AIchallenge::MoveAIplayer(AI_PLAYER player)
{
	//random move algorythm
	m_isRigidCell[player.xPos][player.yPos] = true;
	player.direction = rand() % 4;
	
	//catch loss (fix:wallDrawn)
	if(m_isRigidCell[player.xPos -1][player.yPos] &&
		m_isRigidCell[player.xPos][player.yPos-1] &&
		m_isRigidCell[player.xPos+1][player.yPos] &&
		m_isRigidCell[player.xPos][player.yPos+1])
	{
		GAME_ENGINE->MessageBox(String(player.name) + " lost the game");
		GAME_ENGINE->SetFrameRate(0);
	}
	else
	{
		//catch rigidwall
		switch(player.direction)
		{
		case 0:
			if(player.xPos <= 0) break;
			if(m_isRigidCell[player.xPos -1][player.yPos])break;
			player.xPos--;
			break;
		case 1:
			if(player.yPos <= 0) break;
			if(m_isRigidCell[player.xPos][player.yPos-1])break;
			player.yPos--;
			break;
		case 2:
			if(player.xPos >= GAME_ENGINE->GetWidth() / m_gridSize) break;
			if(m_isRigidCell[player.xPos+1][player.yPos])break;
			player.xPos++;
			break;
		case 3:
			if(player.yPos >= GAME_ENGINE->GetHeight() / m_gridSize) break;
			if(m_isRigidCell[player.xPos][player.yPos+1])break;
			player.yPos++;
			break;
		default:
			break;
		}
	}
	return player;
}

AI_PLAYER AIchallenge::MoveAIplayer(AI_PLAYER player, int pattern)
{
	//Fill Algorythm
	m_isRigidCell[player.xPos][player.yPos] = true;

	if(!m_isRigidCell[player.xPos - 1][player.yPos])
	{
		player.xPos--;
	}
	else if(!m_isRigidCell[player.xPos][player.yPos -1])
	{
		player.yPos--;
	}
	else if(!m_isRigidCell[player.xPos + 1][player.yPos])
	{
		player.xPos++;
	}
	else if(!m_isRigidCell[player.xPos][player.yPos +1])
	{
		player.yPos++;
	}
	catchImmobilised(player);
	return player;
}

void AIchallenge::catchImmobilised(AI_PLAYER player)
{
	if(m_isRigidCell[player.xPos -1][player.yPos] &&
	m_isRigidCell[player.xPos][player.yPos-1] &&
	m_isRigidCell[player.xPos+1][player.yPos] &&
	m_isRigidCell[player.xPos][player.yPos+1])
	{
		GAME_ENGINE->MessageBox(String(player.name) + " lost the game");
		GAME_ENGINE->SetFrameRate(0);
	}
}

void AIchallenge::CallAction(Caller* callerPtr)
{
	// Plaats hier de code die moet uitgevoerd worden wanneer een Caller (zie later) een actie uitvoert
}





