#include "GameCore.h"
#include "Game.h"
#include <Engine.h>
#include <Render.h>
#include <Input.h>
#include <SceneManager.h>
#include <Scene.h>
#include <SceneNode.h>
#include <Camera.h>
#include <ResourceManager.h>
#include <Mesh.h>

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

bool Game::OnInit()
{
	app::res_mgr->AddDir("data");

	Scene* scene = new Scene;
	scene->clear_color = Color(0.1f, 0.1f, 0.1f);
	app::scene_mgr->Add(scene);
	app::scene_mgr->SetActive(scene);

	Camera* camera = new Camera;
	app::scene_mgr->Add(camera);
	app::scene_mgr->SetActive(camera);
	camera->from = Vec3(0, 1, -2);
	camera->to = Vec3(0, 0, 0);

	node = SceneNode::Get();
	node->pos = Vec3::Zero;
	node->rot = Vec3::Zero;
	node->mesh = app::res_mgr->Load<Mesh>("skrzynka.qmsh");
	scene->Add(node);

	return true;
}

void Game::OnUpdate(float dt)
{
	if(app::input->Shortcut(KEY_ALT, Key::F4) || app::input->Down(Key::Escape))
		engine->Shutdown();
	if(app::input->Shortcut(KEY_ALT, Key::Enter))
		engine->SetFullscreen(!engine->IsFullscreen());
	if(app::input->Pressed(Key::N1))
		engine->SetWindowSize(Int2(1024, 768));
	if(app::input->Pressed(Key::N2))
		engine->SetWindowSize(Int2(1280, 1080));
	if(app::input->Shortcut(KEY_CONTROL, Key::U))
		engine->UnlockCursor();

	node->rot.y += dt * 3;
}
