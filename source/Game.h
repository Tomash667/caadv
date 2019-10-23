#pragma once

#include <App.h>

class Game : public App
{
public:
	Game();
	~Game();
	void Run();
	bool OnInit() override;
	void OnCleanup() override;
	void OnUpdate(float dt) override;

	Engine* engine;
	GameCamera* camera;
	FpsCamera* fps_camera;
	GameGui* game_gui;
	Scene* scene;
	SceneNode* node;
	SceneNode* light, *light2, *light3;
	Player* player;
	float light_rot;
};
