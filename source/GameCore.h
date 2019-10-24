#pragma once

#include <EngineCore.h>

class Game;
class GameGui;

struct CharacterController;
struct GameCamera;
struct Player;

enum COLLISION_GROUP
{
	CG_LEVEL = 1 << 0,
	CG_UNIT = 1 << 1
};

extern Game* game;
