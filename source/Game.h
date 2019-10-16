#pragma once

#include <App.h>

class Game : public App
{
public:
	Game();
	~Game();
	void Run();
	void OnUpdate(float dt) override;

private:
	Engine* engine;
};
