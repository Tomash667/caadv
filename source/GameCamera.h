#pragma once

#include <Camera.h>

struct GameCamera : public Camera
{
	GameCamera();
	void Update(float dt, bool allow_mouse);

	SceneNode* target;
	Vec2 rot;
	float height, dist, springiness;
	bool reset;

	static const Vec2 c_angle;
};
