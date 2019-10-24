#include "Pch.h"
#include "GameCore.h"
#include "Player.h"
#include <SceneNode.h>
#include <MeshInstance.h>
#include <ResourceManager.h>
#include <Input.h>
#include "Game.h"
#include "GameCamera.h"
#include "CharacterController.h"

const float RADIUS = 0.3f;
const float HEIGHT = 1.75f;

Player::Player() : anim(ANI_STAND), rot_buf(0.f)
{
	node = SceneNode::Get();
	node->pos = Vec3(0, 0, -2);
	node->rot = Vec3(0, PI, 0);
	node->SetMesh(new MeshInstance(app::res_mgr->Load<Mesh>("human.qmsh")));
	node->mesh_inst->Play("stoi", 0);

	controller = new CharacterController(RADIUS, HEIGHT);
}

Player::~Player()
{
	delete controller;
}

void Player::Update(float dt)
{
	if(controller->canJump() && app::input->Pressed(Key::Spacebar))
		controller->jump();

	Animation new_anim = ANI_STAND;

	const float rot_speed = 5.f;
	const float walk_speed = 2.5f;
	const float anim_run_speed = 4.f;
	const float run_speed = 8.f;

	float required_rot = Clip(-game->camera->rot.x - PI / 2);
	float rot_dif = AngleDiff(node->rot.y, required_rot);
	if(NotZero(rot_dif))
	{
		float best_rot = ShortestArc(node->rot.y, required_rot);
		if(rot_dif <= rot_speed * dt)
			node->rot.y = required_rot;
		else
			node->rot.y = Clip(node->rot.y + best_rot * rot_speed * dt);
		if(best_rot > 0.f)
			rot_buf = 0.1f;
		else
			rot_buf = -0.1f;
	}

	if(rot_buf > 0.f)
	{
		new_anim = ANI_ROTATE_RIGHT;
		rot_buf -= dt;
		if(rot_buf <= 0.f)
			rot_buf = 0;
	}
	else if(rot_buf < 0.f)
	{
		new_anim = ANI_ROTATE_LEFT;
		rot_buf += dt;
		if(rot_buf >= 0.f)
			rot_buf = 0;
	}

	int dir = 0;
	if(app::input->Down(Key::W) || app::input->Down(Key::Up))
		dir += 10;
	if(app::input->Down(Key::S) || app::input->Down(Key::Down))
		dir -= 10;
	if(app::input->Down(Key::A) || app::input->Down(Key::Left))
		dir -= 1;
	if(app::input->Down(Key::D) || app::input->Down(Key::Right))
		dir += 1;
	if(dir == 0)
	{
		controller->setLinearDamping(10.f);
		controller->setWalkDirection(btVector3(0, 0, 0));
	}
	else
	{
		bool run;
		//bool prevent_fall;
		float speed;
		if(app::input->Down(Key::Shift) || dir <= -9)
		{
			speed = walk_speed;
			//prevent_fall = true;
			run = false;
		}
		else
		{
			speed = run_speed;
			//prevent_fall = false;
			run = true;
		}

		float dir_rot;
		switch(dir)
		{
		case -1: // left
			dir_rot = PI * 0 / 4;
			break;
		case -11: // backward left
			dir_rot = PI * 1 / 4;
			break;
		case -10: // backward
			dir_rot = PI * 2 / 4;
			break;
		case -9: // backward right
			dir_rot = PI * 3 / 4;
			break;
		case 1: // right
			dir_rot = PI * 4 / 4;
			break;
		case 11: // forward right
			dir_rot = PI * 5 / 4;
			break;
		case 10: // forward
			dir_rot = PI * 6 / 4;
			break;
		case 9: // forward left
			dir_rot = PI * 7 / 4;
			break;
		}
		dir_rot += -required_rot;

		controller->setLinearDamping(10.f);
		controller->setWalkDirection(btVector3(cos(dir_rot) * speed, 0, sin(dir_rot) * speed));
		//controller->setPreventFall(prevent_fall);
	}

	controller->update(dt);

	const btVector3& velocity = controller->getVelocity();
	float velocity_speed = velocity.length();
	if(velocity_speed >= anim_run_speed)
		new_anim = ANI_RUN;
	else if(velocity_speed >= 0.5f)
	{
		float v_dir = Angle(0, 0, velocity.x(), velocity.z());
		if(AngleDiff(required_rot, v_dir) <= PI / 2)
			new_anim = ANI_WALK;
		else
			new_anim = ANI_WALK_BACK;
	}

	btVector3 pos = controller->getPos();
	node->pos = Vec3(pos.getX(), pos.getY() - HEIGHT / 2, pos.getZ());

	if(node->pos.y < -5.f)
	{
		node->pos = Vec3::Zero;
		controller->reset();
		controller->getObject()->getWorldTransform().getOrigin().setValue(0, HEIGHT / 2, 0);
	}

	if(new_anim != anim)
	{
		switch(new_anim)
		{
		case ANI_STAND:
			node->mesh_inst->Play("stoi", 0, 0);
			break;
		case ANI_WALK:
			node->mesh_inst->Play("idzie", 0, 0);
			break;
		case ANI_WALK_BACK:
			node->mesh_inst->Play("idzie", PLAY_BACK, 0);
			break;
		case ANI_RUN:
			node->mesh_inst->Play("biegnie", 0, 0);
			break;
		case ANI_ROTATE_LEFT:
			node->mesh_inst->Play("w_lewo", 0, 0);
			break;
		case ANI_ROTATE_RIGHT:
			node->mesh_inst->Play("w_prawo", 0, 0);
			break;
		}
		anim = new_anim;
	}
}
