#include "Pch.h"
#include "GameCore.h"
#include "GameCamera.h"
#include <Input.h>
#include <SceneNode.h>

const Vec2 GameCamera::c_angle = Vec2(3.24f, 5.75f);

GameCamera::GameCamera()
{
	dist = 3.5f;
	rot.x = 0;
	rot.y = 4.33499956f;
	height = 1.7f;
	springiness = 40.f;
	reset = true;
}

void GameCamera::Update(float dt, bool allow_mouse)
{
	if(allow_mouse)
	{
		dist -= 0.25f * app::input->GetMouseWheel();
		if(dist < 0.25f)
			dist = 0.25f;
		else if(dist > 5.f)
			dist = 5.f;
		if(app::input->Down(Key::MiddleButton))
		{
			dist = 3.5f;
			rot.y = 4.33499956f;
		}

		Int2 mouse_dif = app::input->GetMouseDif();
		rot.x = Clip(rot.x - float(mouse_dif.x) / 800);
		rot.y = c_angle.Clamp(rot.y - float(mouse_dif.y) / 600);
	}

	Vec3 new_to = target->pos;
	new_to.y += height;

	Vec3 ray(0, -dist, 0);
	Matrix mat = Matrix::Rotation(rot.y, -rot.x - PI / 2, 0);
	ray = Vec3::Transform(ray, mat);
	Vec3 new_from = new_to + ray;

	if(reset)
	{
		from = new_from;
		to = new_to;
		reset = false;
	}
	else
	{
		float d = 1.0f - exp(log(0.5f) * springiness * dt);
		from += (new_from - from) * d;
		to += (new_to - to) * d;
	}
	changed = true;
}
