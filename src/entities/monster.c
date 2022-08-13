/*================================================================
	* entities/monster.c
	*
	* Copyright (c) 2021 Lauri Räsänen
	* ================================

	...
=================================================================*/

#include "monster.h"
#include "../audio/audio_manager.h"
#include "../bsp/bsp_trace.h"
#include "../game/config.h"
#include "../game/console.h"
#include "../game/game_manager.h"
#include "../game/time_manager.h"
#include "../graphics/ui_manager.h"
#include "../util/math.h"
#include "../util/transform.h"
#include "entity.h"
#include <gs/util/gs_idraw.h>

mg_monster_t *mg_monster_new(const char *model_path, const gs_vec3 mins, const gs_vec3 maxs)
{
	mg_monster_t *monster = gs_malloc_init(mg_monster_t);

	mg_cvar_t *vid_width  = mg_cvar("vid_width");
	mg_cvar_t *vid_height = mg_cvar("vid_height");

	*monster = (mg_monster_t){
		.transform	  = gs_vqs_default(),
		.health		  = 100,
		.eye_pos	  = gs_v3(0, 0, maxs.z - MG_MONSTER_EYE_OFFSET),
		.mins		  = gs_v3(mins.x, mins.y, mins.z),
		.maxs		  = gs_v3(maxs.x, maxs.y, maxs.z),
		.last_ground_time = 0,
		.height		  = maxs.z,
		.crouch_height	  = maxs.z * 0.5f,
	};

	monster->model = mg_model_manager_find(model_path);
	if (monster->model == NULL)
	{
		gs_assert(_mg_model_manager_load(model_path, "basic"));
		monster->model = mg_model_manager_find(model_path);
	}
	monster->model_id   = mg_renderer_create_renderable(*monster->model, &monster->transform);
	monster->renderable = mg_renderer_get_renderable(monster->model_id);

	return monster;
}

void mg_monster_free(mg_monster_t *monster)
{
	gs_free(monster);
}

void mg_monster_update(mg_monster_t *monster)
{
	// TODO: time manager, pausing
	if (g_ui_manager->show_cursor) return;
	if (g_game_manager->map == NULL || !g_game_manager->map->valid) return;

	double dt = g_time_manager->delta;
	double pt = g_time_manager->time;

	_mg_monster_think(monster, pt);
	_mg_monster_check_floor(monster);

	// Handle jump and gravity
	if (monster->grounded)
	{
		if (monster->wish_jump)
		{
			_mg_monster_do_jump(monster);
		}
		else
		{
			monster->velocity.z = 0;
		}
	}
	else
	{
		// gravity
		monster->velocity = gs_vec3_add(monster->velocity, gs_vec3_scale(MG_AXIS_DOWN, MG_MONSTER_GRAVITY * dt));
	}

	// Crouching
	if (monster->wish_crouch)
	{
		_mg_monster_crouch(monster, dt);
	}
	else
	{
		_mg_monster_uncrouch(monster, dt);
	}

	// Update velocity
	if (monster->grounded)
	{
		mg_ent_friction(&monster->velocity, MG_MONSTER_STOP_SPEED, MG_MONSTER_FRICTION, dt);
	}

	float move_speed = 0;
	float move_accel = 0;

	if (monster->grounded)
	{
		if (monster->crouched)
		{

			move_speed = mg_lerp(MG_MONSTER_MOVE_SPEED, MG_MONSTER_CROUCH_MOVE_SPEED, monster->crouch_fraction);
			move_accel = MG_MONSTER_ACCEL;
		}
		else
		{
			move_speed = MG_MONSTER_MOVE_SPEED;
			move_accel = MG_MONSTER_ACCEL;
		}
	}
	else
	{
		move_speed = MG_MONSTER_AIR_MOVE_SPEED;
		move_accel = MG_MONSTER_AIR_ACCEL;
	}

	mg_ent_accelerate(
		&monster->velocity,
		monster->wish_move,
		move_speed,
		move_accel,
		MG_MONSTER_MAX_VEL,
		dt);

	// Move
	if (!mg_ent_slidemove(
		    &monster->transform,
		    &monster->velocity,
		    monster->mins,
		    monster->maxs,
		    MG_MONSTER_STEP_HEIGHT,
		    monster->grounded,
		    BSP_CONTENT_CONTENTS_SOLID | BSP_CONTENT_CONTENTS_MONSTERCLIP,
		    dt))
	{
		// FIXME: this just makes you fly up walls...
		// _mg_monster_unstuck(monster);
	}

	// Check out of map bounds
	uint32_t leaf_index   = g_game_manager->map->stats.current_leaf;
	int32_t cluster_index = g_game_manager->map->leaves.data[leaf_index].cluster;
	if (cluster_index < 0)
	{
		mg_println(
			"WARN: monster in invalid leaf, reset to last valid pos: [%f, %f, %f]",
			monster->last_valid_pos.x,
			monster->last_valid_pos.y,
			monster->last_valid_pos.z);
		monster->transform.position = monster->last_valid_pos;
		monster->velocity	    = gs_v3(0, 0, 0);
	}
}

void _mg_monster_think(mg_monster_t *monster, double platform_time)
{
	if (platform_time - monster->last_think_time < MG_MONSTER_THINK_INTERVAL) return;

	monster->last_think_time = platform_time;

	// just move towards player for now
	monster->wish_move = gs_v3(0, 0, 0);
	if (g_game_manager->player != NULL)
	{
		gs_vec3 d		    = gs_vec3_sub(g_game_manager->player->transform.position, monster->transform.position);
		d.z			    = 0;
		d			    = gs_vec3_norm(d);
		monster->transform.rotation = gs_quat_look_rotation(d, MG_AXIS_UP);
		monster->wish_move	    = d;
	}
}

void _mg_monster_do_jump(mg_monster_t *monster)
{
	monster->velocity.z = MG_MONSTER_JUMP_SPEED;
	monster->grounded   = false;
	monster->has_jumped = true;
	if (g_audio_manager != NULL)
		mg_audio_manager_play("monster/jump1.wav", 0.03f);
}

void _mg_monster_check_floor(mg_monster_t *monster)
{
	bsp_trace_t trace = {.map = g_game_manager->map};
	bsp_trace_box(
		&trace,
		monster->transform.position,
		gs_vec3_add(monster->transform.position, gs_vec3_scale(MG_AXIS_DOWN, 2.0f)),
		monster->mins,
		monster->maxs,
		BSP_CONTENT_CONTENTS_SOLID | BSP_CONTENT_CONTENTS_MONSTERCLIP);

	// Don't set grounded if moving up fast enough.
	// (Sliding up a ramp or jumping to a higher level floor)
	// TODO: relative velocity between monster and floor if moving
	float relative_velocity = monster->velocity.z;

	if (trace.fraction < 1.0f && trace.normal.z > 0.7f && relative_velocity < MG_MONSTER_SLIDE_LIMIT)
	{
		monster->grounded	  = true;
		monster->has_jumped	  = false;
		monster->ground_normal	  = trace.normal;
		monster->last_ground_time = g_time_manager->time;

		uint32_t leaf_index   = g_game_manager->map->stats.current_leaf;
		int32_t cluster_index = g_game_manager->map->leaves.data[leaf_index].cluster;
		if (cluster_index >= 0)
		{
			monster->last_valid_pos = monster->transform.position;
		}
	}
	else
	{
		monster->grounded = false;
	}
}

// Can always crouch
void _mg_monster_crouch(mg_monster_t *monster, float delta_time)
{
	if (monster->crouch_fraction == 1.0f) return;

	float32_t crouch_time = MG_MONSTER_CROUCH_TIME;
	if (!monster->grounded) crouch_time = MG_MONSTER_CROUCH_TIME_AIR;

	if (crouch_time > 0.0f)
	{
		float32_t prev_fraction = monster->crouch_fraction;
		monster->crouch_fraction += delta_time / crouch_time;
		if (monster->crouch_fraction > 1.0f) monster->crouch_fraction = 1.0f;

		monster->crouched  = true;
		monster->maxs.z	   = monster->height - monster->crouch_fraction * (monster->height - monster->crouch_height);
		monster->eye_pos.z = monster->maxs.z - MG_MONSTER_EYE_OFFSET;

		// Pull feet up if not on ground
		if (!monster->grounded)
		{
			monster->transform.position.z += (monster->height - monster->crouch_height) * 0.5f * (monster->crouch_fraction - prev_fraction);
		}
	}
	else
	{
		monster->crouched	 = true;
		monster->crouch_fraction = 1.0f;
		monster->maxs.z		 = monster->crouch_height;
		monster->eye_pos.z	 = monster->maxs.z - MG_MONSTER_EYE_OFFSET;

		// Pull feet up if not on ground
		if (!monster->grounded)
		{
			monster->transform.position.z += (monster->height - monster->crouch_height) * 0.5f;
		}
	}
}

// Can uncrouch if enough room
void _mg_monster_uncrouch(mg_monster_t *monster, float delta_time)
{
	if (monster->crouch_fraction == 0.0f) return;

	bsp_trace_t trace = {.map = g_game_manager->map};
	gs_vec3 origin	  = monster->transform.position;
	bool32_t grounded = monster->grounded;

	if (!grounded)
	{
		// Feet will go down if in air, check how much room below monster.
		bsp_trace_box(
			&trace,
			monster->transform.position,
			gs_vec3_scale(mg_get_down(monster->transform.rotation), (monster->height - monster->crouch_height) * 0.5f * monster->crouch_fraction),
			monster->mins,
			monster->maxs,
			BSP_CONTENT_CONTENTS_SOLID | BSP_CONTENT_CONTENTS_MONSTERCLIP);

		origin	 = trace.end;
		grounded = trace.fraction < 1.0f && trace.normal.z > 0.7f;
	}

	// Check above from potential new position.
	bsp_trace_box(
		&trace,
		origin,
		gs_vec3_add(origin, gs_vec3_scale(mg_get_up(monster->transform.rotation), (monster->height - monster->crouch_height) * monster->crouch_fraction)),
		monster->mins, monster->maxs,
		BSP_CONTENT_CONTENTS_SOLID | BSP_CONTENT_CONTENTS_MONSTERCLIP);

	if (trace.fraction == 1.0f)
	{
		// Got room to uncrouch
		float32_t uncrouch_time = MG_MONSTER_UNCROUCH_TIME;
		if (!monster->grounded) uncrouch_time = MG_MONSTER_UNCROUCH_TIME_AIR;

		if (uncrouch_time > 0.0f)
		{
			float32_t prev_fraction = monster->crouch_fraction;
			monster->crouch_fraction -= delta_time / uncrouch_time;
			if (monster->crouch_fraction < 0.0f) monster->crouch_fraction = 0.0f;

			monster->crouched  = monster->crouch_fraction != 0.0f;
			monster->maxs.z	   = monster->height - monster->crouch_fraction * (monster->height - monster->crouch_height);
			monster->eye_pos.z = monster->maxs.z - MG_MONSTER_EYE_OFFSET;
			monster->transform.position.z -= (monster->transform.position.z - origin.z) * (monster->crouch_fraction - prev_fraction);

			if (monster->crouched == 0.0f)
			{
				monster->grounded = grounded;
			}
		}
		else
		{
			monster->crouched	      = false;
			monster->crouch_fraction      = 0.0f;
			monster->maxs.z		      = monster->height;
			monster->eye_pos.z	      = monster->maxs.z - MG_MONSTER_EYE_OFFSET;
			monster->transform.position.z = origin.z;
		}
	}
}

void _mg_monster_unstuck(mg_monster_t *monster)
{
	monster->velocity = gs_v3(0, 0, 0);

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
		// Sweep monster aabb by 1 unit
		start = gs_vec3_add(monster->transform.position, gs_vec3_scale(directions[dir], distance));
		end   = gs_vec3_add(start, directions[dir]);
		bsp_trace_box(&trace, start, end, monster->mins, monster->maxs, BSP_CONTENT_CONTENTS_SOLID | BSP_CONTENT_CONTENTS_MONSTERCLIP);

		if (trace.fraction > 0.0f && !trace.all_solid)
		{
			// Trace ends in a valid position.
			// Trace back towards start so we move
			// the minimum distance to get unstuck.
			gs_vec3 valid_pos = trace.end;
			bsp_trace_box(&trace, trace.end, monster->transform.position, monster->mins, monster->maxs, BSP_CONTENT_CONTENTS_SOLID | BSP_CONTENT_CONTENTS_MONSTERCLIP);
			if (trace.fraction < 1.0f && !trace.all_solid)
			{
				valid_pos = trace.end;
			}

			mg_println(
				"WARN: monster stuck in solid at [%f, %f, %f], freeing to [%f, %f, %f].",
				monster->transform.position.x,
				monster->transform.position.y,
				monster->transform.position.z,
				valid_pos.x,
				valid_pos.y,
				valid_pos.z);
			monster->transform.position = valid_pos;
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
					"WARN: monster stuck in solid at [%f, %f, %f], could not unstuck.",
					monster->transform.position.x,
					monster->transform.position.y,
					monster->transform.position.z);
				break;
			}
		}
	}
}