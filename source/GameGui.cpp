#include "Pch.h"
#include "GameCore.h"
#include "GameGui.h"
#include <Engine.h>
#include <ResourceManager.h>
#include <SceneManager.h>
#include <Input.h>

GameGui::GameGui() : show_info(false)
{
	font = gui->GetFont("Arial", 16, 6);
}

void GameGui::Draw(ControlDrawData*)
{
	cstring text;
	if(show_info)
	{
		text = Format("Fps: %g\n[1] Fog %s\n[2] Lighting %s\n---------------\n[F1] Hide help",
			FLT10(app::engine->GetFps()),
			app::scene_mgr->fog_enabled ? "ON" : "OFF",
			app::scene_mgr->lighting_enabled ? "ON" : "OFF");
	}
	else
		text = Format("Fps: %g\n---------------\n[F1] Show help", FLT10(app::engine->GetFps()));
	Int2 size = font->CalculateSize(text);
	gui->DrawArea(Color(0, 0, 0, 128), Rect(0, 0, size.x + 6, size.y + 6));
	gui->DrawText(font, text, DTF_OUTLINE, Color::White, Rect(2, 2, size.x + 4, size.y + 4));
}

void GameGui::Update(float dt)
{
	if(app::input->Pressed(Key::F1))
		show_info = !show_info;
	if(app::input->Pressed(Key::N1))
		app::scene_mgr->fog_enabled = !app::scene_mgr->fog_enabled;
	if(app::input->Pressed(Key::N2))
		app::scene_mgr->lighting_enabled = !app::scene_mgr->lighting_enabled;
}
