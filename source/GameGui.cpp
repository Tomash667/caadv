#include "Pch.h"
#include "GameCore.h"
#include "GameGui.h"
#include <Engine.h>
#include <ResourceManager.h>

GameGui::GameGui()
{
	font = gui->GetFont("Arial", 16, 6);
}

void GameGui::Draw(ControlDrawData*)
{
	cstring text = Format("Fps: %g", FLT10(app::engine->GetFps()));
	Int2 size = font->CalculateSize(text);
	gui->DrawArea(Color(0, 0, 0, 128), Rect(0, 0, size.x + 6, size.y + 6));
	gui->DrawText(font, text, DTF_OUTLINE, Color::White, Rect(2, 2, size.x + 4, size.y + 4));
}
