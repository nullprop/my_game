/*================================================================
	* entities/monster.h
	*
	* Copyright (c) 2021 Lauri Räsänen
	* ================================

	...
=================================================================*/

#ifndef MG_MONSTER_H
#define MG_MONSTER_H

#include <gs/gs.h>

#include "../bsp/bsp_map.h"
#include "../graphics/model_manager.h"
#include "../graphics/renderer.h"

#define MG_MONSTER_EYE_OFFSET	     4.0f
#define MG_MONSTER_MOVE_SPEED	     240.0f
#define MG_MONSTER_CROUCH_MOVE_SPEED 160.0f
#define MG_MONSTER_ACCEL	     10.0f
#define MG_MONSTER_AIR_MOVE_SPEED    30.0f
#define MG_MONSTER_AIR_ACCEL	     50.0f
#define MG_MONSTER_FRICTION	     8.0f
#define MG_MONSTER_JUMP_SPEED	     250.0f
#define MG_MONSTER_SLIDE_LIMIT	     150.0f
#define MG_MONSTER_STOP_SPEED	     100.0f
#define MG_MONSTER_MAX_VEL	     3500.0f
#define MG_MONSTER_CROUCH_TIME	     0.1f
#define MG_MONSTER_UNCROUCH_TIME     0.1f
// Crouch transitions feel weird in air
#define MG_MONSTER_CROUCH_TIME_AIR   0.0f
#define MG_MONSTER_UNCROUCH_TIME_AIR 0.0f
#define MG_MONSTER_STEP_HEIGHT	     16.0f
#define MG_MONSTER_THINK_INTERVAL    0.5f
#define MG_MONSTER_GRAVITY	     100.0f

typedef struct mg_monster_t
{
	gs_vqs transform;
	int32_t health;
	gs_vec3 velocity;
	gs_vec3 wish_move;
	gs_vec3 mins;
	gs_vec3 maxs;
	gs_vec3 eye_pos;
	gs_vec3 ground_normal;
	bool32_t wish_jump;
	bool32_t wish_crouch;
	bool32_t crouched;
	bool32_t grounded;
	bool32_t has_jumped;
	float32_t crouch_fraction;
	gs_vec3 last_valid_pos;
	double last_ground_time;
	double last_think_time;
	mg_model_t *model;
	uint32_t model_id;
	mg_renderable_t *renderable;
	float height;
	float crouch_height;
} mg_monster_t;

mg_monster_t *mg_monster_new(const char *model_path, const gs_vec3 mins, const gs_vec3 maxs);
void mg_monster_free(mg_monster_t *monster);
void mg_monster_update(mg_monster_t *monster);
void _mg_monster_unstuck(mg_monster_t *monster);
void _mg_monster_think(mg_monster_t *monster, double platform_time);
void _mg_monster_slidemove(mg_monster_t *monster, float delta_time);
void _mg_monster_uncrouch(mg_monster_t *monster, float delta_time);
void _mg_monster_crouch(mg_monster_t *monster, float delta_time);
void _mg_monster_accelerate(mg_monster_t *monster, float delta_time, float move_speed, float acceleration, float max_vel);
void _mg_monster_friction(mg_monster_t *monster, float delta_time);
void _mg_monster_do_jump(mg_monster_t *monster);
void _mg_monster_check_floor(mg_monster_t *monster);

#endif // MG_MONSTER_H