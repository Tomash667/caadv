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
	Logger::SetInstance(new ConsoleLogger);
	engine->SetTitle("Caadv");
	app::render->SetShadersDir("../carpglib/shaders");
	engine->Start(this);
}

void Game::OnUpdate(float dt)
{
	if(app::input->Shortcut(KEY_ALT, Key::F4) || app::input->Down(Key::Escape))
		engine->Shutdown();
	if(app::input->Shortcut(KEY_CONTROL, Key::U))
		engine->UnlockCursor();
	if(app::input->Shortcut(KEY_ALT, Key::Enter))
		engine->SetFullscreen(!engine->IsFullscreen());
	if(app::input->Pressed(Key::N1))
		engine->SetWindowSize(Int2(1024, 768));
	if(app::input->Pressed(Key::N2))
		engine->SetWindowSize(Int2(1280, 1024));
}
