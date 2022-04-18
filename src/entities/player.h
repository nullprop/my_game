/*================================================================
	* entities/player.h
	*
	* Copyright (c) 2021 Lauri Räsänen
	* ================================

	...
=================================================================*/

#ifndef MG_PLAYER_H
#define MG_PLAYER_H

#include <gs/gs.h>

#include "../bsp/bsp_map.h"

#define MG_PLAYER_HEIGHT	    64.0f
#define MG_PLAYER_CROUCH_HEIGHT	    32.0f
#define MG_PLAYER_EYE_OFFSET	    4.0f
#define MG_PLAYER_HALF_WIDTH	    16.0f
#define MG_PLAYER_MOVE_SPEED	    320.0f
#define MG_PLAYER_CROUCH_MOVE_SPEED 160.0f
#define MG_PLAYER_ACCEL		    10.0f
#define MG_PLAYER_AIR_MOVE_SPEED    30.0f
#define MG_PLAYER_AIR_ACCEL	    50.0f
#define MG_PLAYER_FRICTION	    8.0f
#define MG_PLAYER_JUMP_SPEED	    250.0f
#define MG_PLAYER_SLIDE_LIMIT	    150.0f
#define MG_PLAYER_STOP_SPEED	    100.0f
#define MG_PLAYER_MAX_VEL	    3500.0f
#define MG_PLAYER_CROUCH_TIME	    0.1f
#define MG_PLAYER_UNCROUCH_TIME	    0.1f
// Crouch transitions feel weird in air
#define MG_PLAYER_CROUCH_TIME_AIR   0.0f
#define MG_PLAYER_UNCROUCH_TIME_AIR 0.0f
#define MG_PLAYER_STEP_HEIGHT	    16.0f
#define MG_PLAYER_COYOTE_TIME	    0.2f

typedef struct mg_player_camera_t
{
	float32_t pitch;
	float32_t roll;
	gs_camera_t cam;
} mg_player_camera_t;

typedef struct mg_player_t
{
	gs_vqs transform;
	mg_player_camera_t camera;
	gs_camera_t viewmodel_camera;
	float32_t yaw;
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
	gs_vqs viewmodel_transform;
	uint32_t viewmodel_handle;
	double last_ground_time;
} mg_player_t;

mg_player_t *mg_player_new();
void mg_player_free(mg_player_t *player);
void mg_player_update(mg_player_t *player);
void _mg_player_unstuck(mg_player_t *player);
void _mg_player_slidemove(mg_player_t *player, float delta_time);
void _mg_player_uncrouch(mg_player_t *player, float delta_time);
void _mg_player_crouch(mg_player_t *player, float delta_time);
void _mg_player_accelerate(mg_player_t *player, float delta_time, float move_speed, float acceleration, float max_vel);
void _mg_player_friction(mg_player_t *player, float delta_time);
void _mg_player_camera_update(mg_player_t *player);
void _mg_player_get_input(mg_player_t *player, float delta_time);
void _mg_player_do_jump(mg_player_t *player);
void _mg_player_check_floor(mg_player_t *player);

#endif // MG_PLAYER_H