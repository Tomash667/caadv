#pragma once

#include <Control.h>

class GameGui : public Control
{
public:
	GameGui();
	void Draw(ControlDrawData*) override;

private:
	Font* font;
};
