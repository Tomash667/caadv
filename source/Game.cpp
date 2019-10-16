#include "GameCore.h"
#include "Game.h"
#include <Engine.h>
#include <Render.h>
#include <Input.h>

Game::Game() : engine(new Engine)
{
}

Game::~Game()
{
	delete engine;
}

void Game::Run()
{
	engine->SetTitle("Caadv");
	app::render->SetShadersDir("../carpglib/shaders");
	engine->Start(this);
}

void Game::OnUpdate(float dt)
{
	if(app::input->Shortcut(KEY_ALT, Key::F4) || app::input->Down(Key::Escape))
		engine->Shutdown();
}
