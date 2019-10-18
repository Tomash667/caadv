#include "Pch.h"
#include "GameCore.h"
#include <AppEntry.h>
#include "Game.h"

int AppEntry(char* cmd_line)
{
	Game game;
	game.Run();
	return 0;
}
