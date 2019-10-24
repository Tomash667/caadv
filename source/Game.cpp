#include "Pch.h"
#include "GameCore.h"
#include "Game.h"
#include "GameGui.h"
#include <Engine.h>
#include <Render.h>
#include <Input.h>
#include <SceneManager.h>
#include <Scene.h>
#include <SceneNode.h>
#include <FpsCamera.h>
#include <ResourceManager.h>
#include <MeshInstance.h>
#include "Player.h"
#include "GameCamera.h"
#include <Physics.h>

Game* game;

Game::Game() : engine(new Engine), player(nullptr)
{
	game = this;
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

	scene = new Scene;
	scene->ambient_color = Color(0.4f, 0.4f, 0.4f);
	scene->clear_color = Color(0.1f, 0.1f, 0.1f);
	scene->fog_color = Color(0.1f, 0.1f, 0.1f);
	scene->fog_range = Vec2(5, 10);
	app::scene_mgr->Add(scene);
	app::scene_mgr->SetActive(scene);
	app::scene_mgr->normal_map_enabled = false;
	app::scene_mgr->specular_map_enabled = false;

	light = SceneNode::Get();
	light->tint = Vec4(1, 0, 0, 1);
	light->SetLight(5);
	scene->Add(light);

	light2 = SceneNode::Get();
	light2->tint = Vec4(0, 1, 0, 1);
	light2->SetLight(5);
	scene->Add(light2);

	light3 = SceneNode::Get();
	light3->tint = Vec4(0, 0, 1, 1);
	light3->SetLight(5);
	scene->Add(light3);

	node = SceneNode::Get();
	node->pos = Vec3::Zero;
	node->rot = Vec3::Zero;
	node->SetMesh(app::res_mgr->Load<Mesh>("floor.qmsh"));
	node->mesh_inst = nullptr;
	scene->Add(node);

	btBoxShape* shape_floor = new btBoxShape(btVector3(8, 0.01f, 8));
	app::physics->AddShape(shape_floor);

	btCollisionObject* cobj = new btCollisionObject;
	cobj->setCollisionShape(shape_floor);
	cobj->setCollisionFlags(btCollisionObject::CF_STATIC_OBJECT);
	cobj->getWorldTransform().setOrigin(btVector3(0.f, -0.005f, 0.f));
	app::physics->GetWorld()->addCollisionObject(cobj, CG_LEVEL);

	node = SceneNode::Get();
	node->pos = Vec3(-2, 0, 0);
	node->rot = Vec3::Zero;
	node->SetMesh(app::res_mgr->Load<Mesh>("tarcza_strzelnicza.qmsh"));
	scene->Add(node);

	node = SceneNode::Get();
	node->pos = Vec3(-1, 0, 2);
	node->rot = Vec3::Zero;
	node->SetMesh(app::res_mgr->Load<Mesh>("intensiv.qmsh"));
	scene->Add(node);

	node = SceneNode::Get();
	node->pos = Vec3::Zero;
	node->rot = Vec3::Zero;
	node->SetMesh(app::res_mgr->Load<Mesh>("skrzynka.qmsh"));
	scene->Add(node);

	player = new Player;
	scene->Add(player->node);

	camera = new GameCamera;
	camera->target = player->node;
	app::scene_mgr->Add(camera);
	app::scene_mgr->SetActive(camera);

	fps_camera = new FpsCamera;
	app::scene_mgr->Add(fps_camera);

	game_gui = new GameGui();
	app::gui->Add(game_gui);

	light_rot = 0;

	return true;
}

void Game::OnCleanup()
{
	delete player;
}

void Game::OnUpdate(float dt)
{
	if(app::input->Shortcut(KEY_ALT, Key::F4) || app::input->Down(Key::Escape))
		engine->Shutdown();
	if(app::input->Shortcut(KEY_ALT, Key::Enter))
		engine->SetFullscreen(!engine->IsFullscreen());
	if(app::input->Pressed(Key::F5))
		engine->SetWindowSize(Int2(1024, 768));
	if(app::input->Pressed(Key::F9))
		engine->SetWindowSize(Int2(1280, 1080));
	if(app::input->Shortcut(KEY_CONTROL, Key::U))
		engine->UnlockCursor();

	player->Update(dt);

	if(app::scene_mgr->GetActiveCamera() == camera)
		camera->Update(dt, true);
	else
		fps_camera->Update(dt);

	if(!app::input->Down(Key::Backspace))
	{
		node->rot.y += dt * 3;
		app::scene_mgr->Update(dt);

		light_rot += dt;
		scene->light_dir = Vec3(sin(light_rot) * 4, 10, cos(light_rot) * 6).Normalized();

		light->pos = Vec3(cos(light_rot) * 3, 2, sin(light_rot) * 3);

		light2->pos = Vec3(cos(light_rot * 1.5f + PI / 2) * 3, 2, sin(light_rot * 1.5f + PI / 2) * 3);

		light3->pos = Vec3(cos(-light_rot * 0.7f + PI ) * 3, 2, sin(-light_rot * 0.7f + PI) * 3);
	}
}
