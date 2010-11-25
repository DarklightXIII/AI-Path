//-----------------------------------------------------------------
// Game File
// C++ Source - AIchallenge.cpp - version 2010 v2_07
// Copyright Kevin Hoefman - kevin.hoefman@howest.be
// http://www.digitalartsandentertainment.be/
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

AIchallenge::AIchallenge():m_gridSize(15),
							m_default(),
							m_isRigidCell()
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
	GAME_ENGINE->SetWidth(640);
	GAME_ENGINE->SetHeight(480);
    GAME_ENGINE->SetFrameRate(50);
}

void AIchallenge::GameStart()
{
	//initialising default AI
	m_default.xPos = GAME_ENGINE->GetWidth() / 2 / m_gridSize;
	m_default.yPos = GAME_ENGINE->GetHeight() / 2 / m_gridSize;
	m_default.color = RGB(255,150,150);
	
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
		}
		m_isRigidCell[x][0] = true;
		m_isRigidCell[x][GAME_ENGINE->GetHeight() / m_gridSize - 1] = true;
	}
}
void AIchallenge::GameEnd()
{
	// Plaats hier de code die moet uitgevoerd worden bij het afsluiten van het spel 
}
void AIchallenge::GameActivate()
{
	// Plaats hier de code die moet uitgevoerd worden bij het actief worden van het spelvenster 

	/* Voorbeeld:
	GAME_ENGINE->SetSleep(false);
	*/
}
void AIchallenge::GameDeactivate()
{
	// Plaats hier de code die moet uitgevoerd worden bij het inactief worden van het spelvenster 

	/* Voorbeeld:
	GAME_ENGINE->SetSleep(true);
	*/
}

void AIchallenge::MouseButtonAction(bool isLeft, bool isDown, int x, int y, WPARAM wParam)
{	
	// Plaats hier de code die moet uitgevoerd worden wanneer het spel een actie van een muisknop registreert 

	/* Voorbeeld:
	if (isLeft && isDown) // is het een linkermuisclick
	{	
		if ( x > 261 && x < 261 + 117 ) // click ligt binnen de x coordinaten
		{
			if ( y > 182 && y < 182 + 33 ) // click ligt binnen de y coordinaten
			{
				GAME_ENGINE->MessageBox("Geclickt.");
			}
		}
	}
	*/
}
void AIchallenge::MouseMove(int x, int y, WPARAM wParam)
{	
	// Plaats hier de code die moet uitgevoerd worden wanneer de muis over het spelvenster beweegt 

	/* Voorbeeld:
	if (isLeft && isDown) // is het een linkermuisclick
	{	
		if ( x > 261 && x < 261 + 117 ) // click ligt binnen de x coordinaten
		{
			if ( y > 182 && y < 182 + 33 ) // click ligt binnen de y coordinaten
			{
				GAME_ENGINE->MessageBox("Da mouse wuz here.");
			}
		}
	}
	*/
}
void AIchallenge::CheckKeyboard()
{	
	// Hier kun je controleren of een bepaalde toets ingedrukt is
	// Wordt 1x per frame uitgevoerd

	/* Voorbeeld:
	if (GAME_ENGINE->IsKeyDown('K')) xIcon -= xSpeed;
	if (GAME_ENGINE->IsKeyDown('L')) yIcon += xSpeed;
	if (GAME_ENGINE->IsKeyDown('M')) xIcon += xSpeed;
	if (GAME_ENGINE->IsKeyDown('O')) yIcon -= ySpeed;
	*/
}
void AIchallenge::KeyPressed(TCHAR cKey)
{
	// Plaats hier de code die moet uitgevoerd worden wanneer op een opgegeven toets geclickt wordt
	// Wordt uitgevoerd van zodra de opgegeven toets gelost wordt (gebeurt asynchroon)

	/* Voorbeeld:
	switch (cKey)
	{
	case 'K': case VK_LEFT:
		MoveBlock(DIR_LEFT);
		break;
	case 'L': case VK_DOWN:
		MoveBlock(DIR_DOWN);
		break;
	case 'M': case VK_RIGHT:
		MoveBlock(DIR_RIGHT);
		break;
	case 'A': case VK_UP:
		RotateBlock();
		break;
	case VK_ESCAPE:
	}
	*/
}
void AIchallenge::GamePaint(RECT rect)
{
	// Plaats hier de code die moet uitgevoerd worden wanneer het spel zich tekent naar het scherm 
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



	//Move the AI and assign rigidBody
	if(_fpst % 15== 0)
	{
		m_isRigidCell[m_default.xPos][m_default.yPos] = true;
		
		m_default = MoveAIplayer(m_default);
	}

	//Draw the rigid cells
	DrawRigidBodies();

	//Draw the AI
	DrawAIplayer(m_default);

	_fpst++;
}

void AIchallenge::DrawAIplayer(AI_PLAYER player)
{
	GAME_ENGINE->SetColor(player.color);
	GAME_ENGINE->FillRect(player.xPos * m_gridSize + 1, player.yPos * m_gridSize + 1,m_gridSize -1,m_gridSize-1);
}

void AIchallenge::DrawRigidBodies()
{
	GAME_ENGINE->SetColor(RGB(180,180,255));
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
	//if(m_isRigidCell[]
	player.xPos++;

	return player;
}

void AIchallenge::CallAction(Caller* callerPtr)
{
	// Plaats hier de code die moet uitgevoerd worden wanneer een Caller (zie later) een actie uitvoert
}





