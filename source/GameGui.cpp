#include "Pch.h"
#include "GameCore.h"
#include "GameGui.h"
#include <Engine.h>
#include <ResourceManager.h>
#include <SceneManager.h>
#include <Input.h>
#include <Scene.h>
#include "Game.h"
#include <FpsCamera.h>
#include "GameCamera.h"

GameGui::GameGui() : scene(game->scene), show_info(false)
{
	font = gui->GetFont("Arial", 16, 6);
}

void GameGui::Draw(ControlDrawData*)
{
	cstring text;
	if(show_info)
	{
		text = Format("Fps: %g\n"
			"[WSAD] Move\n"
			"[Spacebar] Jump\n"
			"[Shift] Walk\n"
			"[Backspace] Stop animations\n"
			"[F] Toggle fps camera\n"
			"---------------\n"
			"[1] Fog %s\n"
			"[2] Dir light %s\n"
			"[3] Point light %s\n"
			"[4] Normal map %s\n"
			"[5] Specular map %s\n"
			"---------------\n"
			"[F1] Hide help",
			FLT10(app::engine->GetFps()),
			app::scene_mgr->fog_enabled ? "ON" : "OFF",
			scene->use_dir_light ? "ON" : "OFF",
			scene->use_point_light ? "ON" : "OFF",
			app::scene_mgr->normal_map_enabled ? "ON" : "OFF",
			app::scene_mgr->specular_map_enabled ? "ON" : "OFF");
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
	{
		scene->use_dir_light = !scene->use_dir_light;
		scene->use_point_light = false;
	}
	if(app::input->Pressed(Key::N3))
	{
		scene->use_point_light = !scene->use_point_light;
		scene->use_dir_light = false;
	}
	if(app::input->Pressed(Key::N4))
		app::scene_mgr->normal_map_enabled = !app::scene_mgr->normal_map_enabled;
	if(app::input->Pressed(Key::N5))
		app::scene_mgr->specular_map_enabled = !app::scene_mgr->specular_map_enabled;
	if(app::input->Pressed(Key::F))
	{
		if(app::scene_mgr->GetActiveCamera() == game->camera)
		{
			app::scene_mgr->SetActive(game->fps_camera);
			game->fps_camera->from = game->camera->from;
			game->fps_camera->LookAt(game->camera->to);
		}
		else
			app::scene_mgr->SetActive(game->camera);
	}
}
