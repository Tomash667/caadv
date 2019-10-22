#pragma once

#include <App.h>

class Game : public App
{
public:
	Game();
	~Game();
	void Run();
	bool OnInit() override;
	void OnUpdate(float dt) override;

private:
	Engine* engine;
	FpsCamera* camera;
	GameGui* game_gui;
	Scene* scene;
	SceneNode* node;
	SceneNode* light, *light2, *light3;
	float light_rot;
};
