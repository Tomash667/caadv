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
	GameGui* game_gui;
	SceneNode* node;
};
