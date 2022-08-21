/*================================================================
	* entities/player.c
	*
	* Copyright (c) 2021 Lauri Räsänen
	* ================================

	...
=================================================================*/

#include "player.h"
#include "../audio/audio_manager.h"
#include "../bsp/bsp_trace.h"
#include "../game/config.h"
#include "../game/console.h"
#include "../game/game_manager.h"
#include "../game/time_manager.h"
#include "../graphics/model_manager.h"
#include "../graphics/renderer.h"
#include "../graphics/ui_manager.h"
#include "../util/camera.h"
#include "../util/math.h"
#include "entity.h"

#include <gs/util/gs_idraw.h>

mg_player_t *mg_player_new()
{
	mg_player_t *player = gs_malloc_init(mg_player_t);

	mg_cvar_t *vid_width  = mg_cvar("vid_width");
	mg_cvar_t *vid_height = mg_cvar("vid_height");

	*player = (mg_player_t){
		.transform = gs_vqs_default(),
		.camera	   = {
			   .cam = {
				   .aspect_ratio = (float)vid_width->value.i / vid_height->value.i,
				   .far_plane	 = 10000.0f,
				   .fov		 = mg_cvar("r_fov")->value.i,
				   .near_plane	 = 0.1f,
				   .proj_type	 = GS_PROJECTION_TYPE_PERSPECTIVE,
			   },
		   },
		.viewmodel_camera = {
			.aspect_ratio = (float)vid_width->value.i / vid_height->value.i,
			.far_plane    = 1000.0f,
			.fov	      = 60.0f,
			.near_plane   = 0,
			.proj_type    = GS_PROJECTION_TYPE_PERSPECTIVE,
		},
		.health		  = 100,
		.eye_pos	  = gs_v3(0, 0, MG_PLAYER_HEIGHT - MG_PLAYER_EYE_OFFSET),
		.mins		  = gs_v3(-MG_PLAYER_HALF_WIDTH, -MG_PLAYER_HALF_WIDTH, 0),
		.maxs		  = gs_v3(MG_PLAYER_HALF_WIDTH, MG_PLAYER_HALF_WIDTH, MG_PLAYER_HEIGHT),
		.last_ground_time = 0,
		.weapon_current	  = -1,
	};

	for (size_t i = 0; i < MG_WEAPON_COUNT; i++)
	{
		player->weapons[i] = mg_weapon_create(i);
		mg_renderer_set_hidden(player->weapons[i]->renderable_id, true);
	}

	_mg_player_camera_update(player);

	return player;
}

void mg_player_free(mg_player_t *player)
{
	for (size_t i = 0; i < MG_WEAPON_COUNT; i++)
	{
		mg_weapon_free(player->weapons[i]);
	}

	gs_free(player);
}

void mg_player_update(mg_player_t *player)
{
	// TODO: time manager, pausing
	if (g_ui_manager->show_cursor) return;
	if (g_game_manager->map == NULL || !g_game_manager->map->valid) return;

	double dt = g_time_manager->delta;
	double pt = g_time_manager->time;

	_mg_player_check_floor(player);

	// Handle jump and gravity
	if (player->grounded)
	{
		if (player->wish_jump)
		{
			_mg_player_do_jump(player);
		}
		else
		{
			player->velocity.z = 0;
		}
	}
	else
	{
		// gravity
		player->velocity = gs_vec3_add(player->velocity, gs_vec3_scale(MG_AXIS_DOWN, MG_PLAYER_GRAVITY * dt));

		// coyote
		if (player->wish_jump && !player->has_jumped && pt - player->last_ground_time <= MG_PLAYER_COYOTE_TIME)
		{
			_mg_player_do_jump(player);
		}
	}

	// Crouching
	if (player->wish_crouch)
	{
		_mg_player_crouch(player, dt);
	}
	else
	{
		_mg_player_uncrouch(player, dt);
	}

	// Update velocity
	if (player->grounded)
	{
		mg_ent_friction(&player->velocity, MG_PLAYER_STOP_SPEED, MG_PLAYER_FRICTION, dt);
	}

	float move_speed = 0;
	float move_accel = 0;

	if (player->grounded)
	{
		if (player->crouched)
		{

			move_speed = mg_lerp(MG_PLAYER_MOVE_SPEED, MG_PLAYER_CROUCH_MOVE_SPEED, player->crouch_fraction);
			move_accel = MG_PLAYER_ACCEL;
		}
		else
		{
			move_speed = MG_PLAYER_MOVE_SPEED;
			move_accel = MG_PLAYER_ACCEL;
		}
	}
	else
	{
		move_speed = MG_PLAYER_AIR_MOVE_SPEED;
		move_accel = MG_PLAYER_AIR_ACCEL;
	}

	mg_ent_accelerate(
		&player->velocity,
		player->wish_move,
		move_speed,
		move_accel,
		MG_PLAYER_MAX_VEL,
		dt);

	// Move
	if (!mg_ent_slidemove(
		    &player->transform,
		    &player->velocity,
		    player->mins,
		    player->maxs,
		    MG_PLAYER_STEP_HEIGHT,
		    player->grounded,
		    BSP_CONTENT_CONTENTS_SOLID | BSP_CONTENT_CONTENTS_PLAYERCLIP,
		    dt))
	{
		// FIXME: this just makes you fly up walls...
		// _mg_player_unstuck(player);
	}

	_mg_player_camera_update(player);

	// Check out of map bounds
	uint32_t leaf_index   = g_game_manager->map->stats.current_leaf;
	int32_t cluster_index = g_game_manager->map->leaves.data[leaf_index].cluster;
	if (cluster_index < 0)
	{
		mg_println(
			"WARN: player in invalid leaf, reset to last valid pos: [%f, %f, %f]",
			player->last_valid_pos.x,
			player->last_valid_pos.y,
			player->last_valid_pos.z);
		player->transform.position = player->last_valid_pos;
		player->velocity	   = gs_v3(0, 0, 0);
	}

	if (player->wish_shoot)
	{
		_mg_player_shoot(player);
	}
}

void mg_player_switch_weapon(mg_player_t *player, int32_t slot)
{
	if (slot >= MG_WEAPON_COUNT)
	{
		gs_println("WARN: mg_player_switch_weapon: slot %d higher than max %d", slot, MG_WEAPON_COUNT - 1);
		return;
	}

	if (player->weapon_current >= 0)
	{
		mg_renderer_set_hidden(player->weapons[player->weapon_current]->renderable_id, true);
	}

	player->weapon_current = slot;

	if (player->weapon_current >= 0)
	{
		mg_renderer_set_hidden(player->weapons[player->weapon_current]->renderable_id, false);
	}
}

void _mg_player_do_jump(mg_player_t *player)
{
	player->velocity.z = MG_PLAYER_JUMP_SPEED;
	player->grounded   = false;
	player->has_jumped = true;
	if (g_audio_manager != NULL)
		mg_audio_manager_play("player/jump1.wav", 0.03f);
}

void _mg_player_check_floor(mg_player_t *player)
{
	bsp_trace_t trace = {.map = g_game_manager->map};
	bsp_trace_box(
		&trace,
		player->transform.position,
		gs_vec3_add(player->transform.position, gs_vec3_scale(MG_AXIS_DOWN, 2.0f)),
		player->mins,
		player->maxs,
		BSP_CONTENT_CONTENTS_SOLID | BSP_CONTENT_CONTENTS_PLAYERCLIP);

	// Don't set grounded if moving up fast enough.
	// (Sliding up a ramp or jumping to a higher level floor)
	// TODO: relative velocity between player and floor if moving
	float relative_velocity = player->velocity.z;

	if (trace.fraction < 1.0f && trace.normal.z > 0.7f && relative_velocity < MG_PLAYER_SLIDE_LIMIT)
	{
		player->grounded	 = true;
		player->has_jumped	 = false;
		player->ground_normal	 = trace.normal;
		player->last_ground_time = g_time_manager->time;

		uint32_t leaf_index   = g_game_manager->map->stats.current_leaf;
		int32_t cluster_index = g_game_manager->map->leaves.data[leaf_index].cluster;
		if (cluster_index >= 0)
		{
			player->last_valid_pos = player->transform.position;
		}
	}
	else
	{
		player->grounded = false;
	}
}

void _mg_player_camera_update(mg_player_t *player)
{
	player->camera.cam.transform = gs_vqs_absolute_transform(
		&(gs_vqs){
			.position = player->eye_pos,
			.rotation = gs_quat_mul(
				gs_quat_angle_axis(gs_deg2rad(-player->camera.pitch), MG_AXIS_RIGHT),
				gs_quat_angle_axis(gs_deg2rad(player->camera.roll), MG_AXIS_FORWARD)),
			.scale = gs_v3(1.0f, 1.0f, 1.0f),
		},
		&player->transform);

	if (player->weapon_current >= 0)
	{
		player->weapons[player->weapon_current]->transform = gs_vqs_absolute_transform(
			&(gs_vqs){
				.position = gs_v3(
					mg_cvar("r_viewmodel_pos_x")->value.f,
					mg_cvar("r_viewmodel_pos_y")->value.f,
					mg_cvar("r_viewmodel_pos_z")->value.f),
				.rotation = gs_quat_default(),
				.scale	  = gs_v3(
					   mg_cvar("r_viewmodel_scale_x")->value.f,
					   mg_cvar("r_viewmodel_scale_y")->value.f,
					   mg_cvar("r_viewmodel_scale_z")->value.f),
			},
			&player->camera.cam.transform);
	}
}

// Can always crouch
void _mg_player_crouch(mg_player_t *player, float delta_time)
{
	if (player->crouch_fraction == 1.0f) return;

	float32_t crouch_time = MG_PLAYER_CROUCH_TIME;
	if (!player->grounded) crouch_time = MG_PLAYER_CROUCH_TIME_AIR;

	if (crouch_time > 0.0f)
	{
		float32_t prev_fraction = player->crouch_fraction;
		player->crouch_fraction += delta_time / crouch_time;
		if (player->crouch_fraction > 1.0f) player->crouch_fraction = 1.0f;

		player->crouched  = true;
		player->maxs.z	  = MG_PLAYER_HEIGHT - player->crouch_fraction * (MG_PLAYER_HEIGHT - MG_PLAYER_CROUCH_HEIGHT);
		player->eye_pos.z = player->maxs.z - MG_PLAYER_EYE_OFFSET;

		// Pull feet up if not on ground
		if (!player->grounded)
		{
			player->transform.position.z += (MG_PLAYER_HEIGHT - MG_PLAYER_CROUCH_HEIGHT) * 0.5f * (player->crouch_fraction - prev_fraction);
		}
	}
	else
	{
		player->crouched	= true;
		player->crouch_fraction = 1.0f;
		player->maxs.z		= MG_PLAYER_CROUCH_HEIGHT;
		player->eye_pos.z	= player->maxs.z - MG_PLAYER_EYE_OFFSET;

		// Pull feet up if not on ground
		if (!player->grounded)
		{
			player->transform.position.z += (MG_PLAYER_HEIGHT - MG_PLAYER_CROUCH_HEIGHT) * 0.5f;
		}
	}
}

// Can uncrouch if enough room
void _mg_player_uncrouch(mg_player_t *player, float delta_time)
{
	if (player->crouch_fraction == 0.0f) return;

	bsp_trace_t trace = {.map = g_game_manager->map};
	gs_vec3 origin	  = player->transform.position;
	bool32_t grounded = player->grounded;

	if (!grounded)
	{
		// Feet will go down if in air, check how much room below player.
		bsp_trace_box(
			&trace,
			player->transform.position,
			gs_vec3_scale(mg_get_down(player->transform.rotation), (MG_PLAYER_HEIGHT - MG_PLAYER_CROUCH_HEIGHT) * 0.5f * player->crouch_fraction),
			player->mins,
			player->maxs,
			BSP_CONTENT_CONTENTS_SOLID | BSP_CONTENT_CONTENTS_PLAYERCLIP);

		origin	 = trace.end;
		grounded = trace.fraction < 1.0f && trace.normal.z > 0.7f;
	}

	// Check above from potential new position.
	bsp_trace_box(
		&trace,
		origin,
		gs_vec3_add(origin, gs_vec3_scale(mg_get_up(player->transform.rotation), (MG_PLAYER_HEIGHT - MG_PLAYER_CROUCH_HEIGHT) * player->crouch_fraction)),
		player->mins, player->maxs,
		BSP_CONTENT_CONTENTS_SOLID | BSP_CONTENT_CONTENTS_PLAYERCLIP);

	if (trace.fraction == 1.0f)
	{
		// Got room to uncrouch
		float32_t uncrouch_time = MG_PLAYER_UNCROUCH_TIME;
		if (!player->grounded) uncrouch_time = MG_PLAYER_UNCROUCH_TIME_AIR;

		if (uncrouch_time > 0.0f)
		{
			float32_t prev_fraction = player->crouch_fraction;
			player->crouch_fraction -= delta_time / uncrouch_time;
			if (player->crouch_fraction < 0.0f) player->crouch_fraction = 0.0f;

			player->crouched  = player->crouch_fraction != 0.0f;
			player->maxs.z	  = MG_PLAYER_HEIGHT - player->crouch_fraction * (MG_PLAYER_HEIGHT - MG_PLAYER_CROUCH_HEIGHT);
			player->eye_pos.z = player->maxs.z - MG_PLAYER_EYE_OFFSET;
			player->transform.position.z -= (player->transform.position.z - origin.z) * (player->crouch_fraction - prev_fraction);

			if (player->crouched == 0.0f)
			{
				player->grounded = grounded;
			}
		}
		else
		{
			player->crouched	     = false;
			player->crouch_fraction	     = 0.0f;
			player->maxs.z		     = MG_PLAYER_HEIGHT;
			player->eye_pos.z	     = player->maxs.z - MG_PLAYER_EYE_OFFSET;
			player->transform.position.z = origin.z;
		}
	}
}

void _mg_player_unstuck(mg_player_t *player)
{
	player->velocity = gs_v3(0, 0, 0);

	// Directions to attempt to teleport to,
	// in listed order.
	gs_vec3 directions[6] = {
		MG_AXIS_UP,
		MG_AXIS_FORWARD,
		MG_AXIS_RIGHT,
		MG_AXIS_BACKWARD,
		MG_AXIS_LEFT,
		MG_AXIS_DOWN,
	};

	float distance	   = 0.0f;
	float increment	   = 64.0f;
	float max_distance = increment * 10;
	uint32_t dir	   = 0;
	gs_vec3 start	   = gs_v3(0, 0, 0);
	gs_vec3 end	   = gs_v3(0, 0, 0);
	bsp_trace_t trace  = {.map = g_game_manager->map};

	while (true)
	{
		// Sweep player aabb by 1 unit
		start = gs_vec3_add(player->transform.position, gs_vec3_scale(directions[dir], distance));
		end   = gs_vec3_add(start, directions[dir]);
		bsp_trace_box(&trace, start, end, player->mins, player->maxs, BSP_CONTENT_CONTENTS_SOLID | BSP_CONTENT_CONTENTS_PLAYERCLIP);

		if (trace.fraction > 0.0f && !trace.all_solid)
		{
			// Trace ends in a valid position.
			// Trace back towards start so we move
			// the minimum distance to get unstuck.
			gs_vec3 valid_pos = trace.end;
			bsp_trace_box(&trace, trace.end, player->transform.position, player->mins, player->maxs, BSP_CONTENT_CONTENTS_SOLID | BSP_CONTENT_CONTENTS_PLAYERCLIP);
			if (trace.fraction < 1.0f && !trace.all_solid)
			{
				valid_pos = trace.end;
			}

			mg_println(
				"WARN: player stuck in solid at [%f, %f, %f], freeing to [%f, %f, %f].",
				player->transform.position.x,
				player->transform.position.y,
				player->transform.position.z,
				valid_pos.x,
				valid_pos.y,
				valid_pos.z);
			player->transform.position = valid_pos;
			break;
		}

		// Swap to the next direction,
		// increment distance if looped through all.
		dir++;
		if (dir >= 6)
		{
			dir = 0;
			distance += increment;
			if (distance > max_distance)
			{
				mg_println(
					"WARN: player stuck in solid at [%f, %f, %f], could not unstuck.",
					player->transform.position.x,
					player->transform.position.y,
					player->transform.position.z);
				break;
			}
		}
	}
}

void _mg_player_shoot(mg_player_t *player)
{
	if (player->weapon_current < 0 || player->weapon_current >= MG_WEAPON_COUNT)
	{
		return;
	}

	mg_weapon_t *weapon = player->weapons[player->weapon_current];

	if (weapon->ammo_current <= 0)
	{
		return;
	}

	weapon->ammo_current--;

	// TODO
}