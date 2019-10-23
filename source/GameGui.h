#pragma once

#include <Control.h>

class GameGui : public Control
{
public:
	GameGui();
	void Draw(ControlDrawData*) override;
	void Update(float dt) override;

private:
	Font* font;
	Scene* scene;
	bool show_info;
};
