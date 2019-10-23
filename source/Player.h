#pragma once

struct Player
{
	enum Animation
	{
		ANI_STAND,
		ANI_WALK,
		ANI_WALK_BACK,
		ANI_RUN,
		ANI_ROTATE_LEFT,
		ANI_ROTATE_RIGHT
	};

	Player();
	~Player();
	void Update(float dt);

	SceneNode* node;
	CharacterController* controller;
	Animation anim;
	float rot_buf;
};
