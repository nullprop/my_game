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
#include "../graphics/ui_manager.h"
#include "../util/math.h"
#include "../util/transform.h"
#include <gs/util/gs_idraw.h>

mg_monster_t *mg_monster_new(const char *model_path)
{
	mg_monster_t *monster = gs_malloc_init(mg_monster_t);

	mg_cvar_t *vid_width  = mg_cvar("vid_width");
	mg_cvar_t *vid_height = mg_cvar("vid_height");

	*monster = (mg_monster_t){
		.transform	  = gs_vqs_default(),
		.health		  = 100,
		.eye_pos	  = gs_v3(0, 0, MG_MONSTER_HEIGHT - MG_MONSTER_EYE_OFFSET),
		.mins		  = gs_v3(-MG_MONSTER_HALF_WIDTH, -MG_MONSTER_HALF_WIDTH, 0),
		.maxs		  = gs_v3(MG_MONSTER_HALF_WIDTH, MG_MONSTER_HALF_WIDTH, MG_MONSTER_HEIGHT),
		.last_ground_time = 0,
	};
	monster->transform.scale = gs_v3(10.0f, 10.0f, 10.0f);

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

	gs_platform_t *platform = gs_subsystem(platform);
	float dt		= platform->time.delta;
	double pt		= gs_platform_elapsed_time();

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
	_mg_monster_friction(monster, dt);

	if (monster->grounded)
	{
		if (monster->crouched)
		{
			_mg_monster_accelerate(monster, dt, mg_lerp(MG_MONSTER_MOVE_SPEED, MG_MONSTER_CROUCH_MOVE_SPEED, monster->crouch_fraction), MG_MONSTER_ACCEL, MG_MONSTER_MAX_VEL);
		}
		else
		{
			_mg_monster_accelerate(monster, dt, MG_MONSTER_MOVE_SPEED, MG_MONSTER_ACCEL, MG_MONSTER_MAX_VEL);
		}
	}
	else
	{
		_mg_monster_accelerate(monster, dt, MG_MONSTER_AIR_MOVE_SPEED, MG_MONSTER_AIR_ACCEL, MG_MONSTER_MAX_VEL);
	}

	// Move
	_mg_monster_slidemove(monster, dt);

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
		mg_audio_manager_play("sound/monster/jump1.wav", 0.03f);
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
		monster->last_ground_time = gs_platform_elapsed_time();

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

void _mg_monster_friction(mg_monster_t *monster, float delta_time)
{
	if (monster->grounded == false) return;

	float vel2 = gs_vec3_len2(monster->velocity);
	if (vel2 < GS_EPSILON)
	{
		monster->velocity = gs_v3(0, 0, 0);
		return;
	}

	float vel      = sqrtf(vel2);
	float loss     = fmaxf(vel, MG_MONSTER_STOP_SPEED) * MG_MONSTER_FRICTION * delta_time;
	float fraction = fmaxf(0, vel - loss) / vel;

	monster->velocity = gs_vec3_scale(monster->velocity, fraction);
}

void _mg_monster_accelerate(mg_monster_t *monster, float delta_time, float move_speed, float acceleration, float max_vel)
{
	// Velocity projected on to wished direction,
	// positive if in the same direction.
	float proj_vel = gs_vec3_dot(monster->velocity, monster->wish_move);

	// The max amount to change by,
	// won't accelerate past move_speed.
	float change = move_speed - proj_vel;
	if (change <= 0) return;

	// The actual acceleration
	float accel = acceleration * move_speed * delta_time;

	// Clamp to max change
	if (accel > change) accel = change;

	monster->velocity = gs_vec3_add(monster->velocity, gs_vec3_scale(monster->wish_move, accel));

	// Clamp to max vel per axis
	monster->velocity.x = fminf(MG_MONSTER_MAX_VEL, fmaxf(monster->velocity.x, -MG_MONSTER_MAX_VEL));
	monster->velocity.y = fminf(MG_MONSTER_MAX_VEL, fmaxf(monster->velocity.y, -MG_MONSTER_MAX_VEL));
	monster->velocity.z = fminf(MG_MONSTER_MAX_VEL, fmaxf(monster->velocity.z, -MG_MONSTER_MAX_VEL));
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
		monster->maxs.z	   = MG_MONSTER_HEIGHT - monster->crouch_fraction * (MG_MONSTER_HEIGHT - MG_MONSTER_CROUCH_HEIGHT);
		monster->eye_pos.z = monster->maxs.z - MG_MONSTER_EYE_OFFSET;

		// Pull feet up if not on ground
		if (!monster->grounded)
		{
			monster->transform.position.z += (MG_MONSTER_HEIGHT - MG_MONSTER_CROUCH_HEIGHT) * 0.5f * (monster->crouch_fraction - prev_fraction);
		}
	}
	else
	{
		monster->crouched	 = true;
		monster->crouch_fraction = 1.0f;
		monster->maxs.z		 = MG_MONSTER_CROUCH_HEIGHT;
		monster->eye_pos.z	 = monster->maxs.z - MG_MONSTER_EYE_OFFSET;

		// Pull feet up if not on ground
		if (!monster->grounded)
		{
			monster->transform.position.z += (MG_MONSTER_HEIGHT - MG_MONSTER_CROUCH_HEIGHT) * 0.5f;
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
			gs_vec3_scale(mg_get_down(monster->transform.rotation), (MG_MONSTER_HEIGHT - MG_MONSTER_CROUCH_HEIGHT) * 0.5f * monster->crouch_fraction),
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
		gs_vec3_add(origin, gs_vec3_scale(mg_get_up(monster->transform.rotation), (MG_MONSTER_HEIGHT - MG_MONSTER_CROUCH_HEIGHT) * monster->crouch_fraction)),
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
			monster->maxs.z	   = MG_MONSTER_HEIGHT - monster->crouch_fraction * (MG_MONSTER_HEIGHT - MG_MONSTER_CROUCH_HEIGHT);
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
			monster->maxs.z		      = MG_MONSTER_HEIGHT;
			monster->eye_pos.z	      = monster->maxs.z - MG_MONSTER_EYE_OFFSET;
			monster->transform.position.z = origin.z;
		}
	}
}

void _mg_monster_slidemove(mg_monster_t *monster, float delta_time)
{
	uint16_t current_iter = 0;
	uint16_t max_iter     = 10;
	gs_vec3 start;
	gs_vec3 end;
	bsp_trace_t trace = {.map = g_game_manager->map};
	float32_t prev_frac;
	gs_vec3 prev_normal;

	while (delta_time > 0)
	{
		if (gs_vec3_len2(monster->velocity) == 0)
		{
			break;
		}

		// Sanity check for infinite loops,
		// shouldn't really happen.
		if (current_iter >= max_iter)
		{
			break;
		}
		current_iter++;

		start = monster->transform.position;
		end   = gs_vec3_add(start, gs_vec3_scale(monster->velocity, delta_time));

		bsp_trace_box(
			&trace,
			start,
			end,
			monster->mins,
			monster->maxs,
			BSP_CONTENT_CONTENTS_SOLID | BSP_CONTENT_CONTENTS_MONSTERCLIP);

		if (trace.start_solid)
		{
			// Stuck in a solid, try to get out
			if (trace.all_solid)
			{
				// FIXME: this just makes you fly up walls...
				//_mg_monster_unstuck(monster);
			}
			else
			{
				mg_println(
					"WARN: monster start solid but not stuck: [%f, %f, %f] -> [%f, %f, %f]",
					monster->transform.position.x,
					monster->transform.position.y,
					monster->transform.position.z,
					trace.end.x,
					trace.end.y,
					trace.end.z);
				monster->transform.position = trace.end;
				monster->velocity	    = gs_v3(0, 0, 0);
			}
			break;
		}

		if (trace.fraction > 0)
		{
			// Move as far as we can
			monster->transform.position = trace.end;
		}

		bool is_done	= false;
		gs_vec3 end_pos = trace.end;

		if (trace.fraction == 1.0f)
		{
			// Moved all the way
			delta_time = 0;
			is_done	   = true;
			goto step_down;
		}

		delta_time -= delta_time * trace.fraction;

		// Colliding with something, check if we can move further
		// by stepping up or down before clipping velocity to the plane.
		gs_vec3 hit_normal = trace.normal;

	step_up:
		// TODO
		// Known issues:
		// 	Hugging a wall stops monster from going up stairs.
		// 	The forward trace will collide with the wall if velocity
		// 	is angled towards it.
		// 	Potential solution: If forward trace collides with anything,
		// 	project velocity to normal and try again?

		// Check above
		bsp_trace_box(
			&trace,
			end_pos,
			gs_vec3_add(end_pos, gs_v3(0, 0, MG_MONSTER_STEP_HEIGHT)),
			monster->mins,
			monster->maxs,
			BSP_CONTENT_CONTENTS_SOLID | BSP_CONTENT_CONTENTS_MONSTERCLIP);
		if (trace.fraction <= 0)
			goto step_down;

		// Check forward
		gs_vec3 horizontal_vel = monster->velocity;
		horizontal_vel.z       = 0;
		horizontal_vel	       = gs_vec3_scale(horizontal_vel, delta_time);
		// Player can be epsilon from an edge on either axis,
		// and will need to go epsilon over it for downwards trace to hit anything.
		// Scaling both axes to a min of 2 * epsilon (if not 0) allows monster to
		// step up stairs even if moving at a very shallow angle towards them.
		// While this can make the monster move a very tiny distance further,
		// it's imperceptible in testing and preferred to getting stuck on a stair.
		if (fabs(horizontal_vel.x) > GS_EPSILON && fabs(horizontal_vel.x) < BSP_TRACE_EPSILON * 2.0f)
			horizontal_vel.x = (horizontal_vel.x / fabs(horizontal_vel.x)) * BSP_TRACE_EPSILON * 2.0f;
		if (fabs(horizontal_vel.y) > GS_EPSILON && fabs(horizontal_vel.y) < BSP_TRACE_EPSILON * 2.0f)
			horizontal_vel.y = (horizontal_vel.y / fabs(horizontal_vel.y)) * BSP_TRACE_EPSILON * 2.0f;

		bsp_trace_box(
			&trace,
			trace.end,
			gs_vec3_add(trace.end, horizontal_vel),
			monster->mins,
			monster->maxs,
			BSP_CONTENT_CONTENTS_SOLID | BSP_CONTENT_CONTENTS_MONSTERCLIP);
		if (trace.fraction <= 0)
			goto step_down;

		float forward_frac = trace.fraction;

		// Move down
		bsp_trace_box(
			&trace,
			trace.end,
			gs_vec3_add(trace.end, gs_v3(0, 0, -MG_MONSTER_STEP_HEIGHT)),
			monster->mins,
			monster->maxs,
			BSP_CONTENT_CONTENTS_SOLID | BSP_CONTENT_CONTENTS_MONSTERCLIP);
		if (trace.fraction == 1.0f || trace.end.z <= end_pos.z || trace.normal.z <= 0.7f)
			goto step_down;

		monster->transform.position = trace.end;
		delta_time -= delta_time * forward_frac;
		continue;

	step_down:
		// We're already going 'forward' as much as we can,
		// try to stick to the floor if grounded before.
		if (!monster->grounded)
			goto slide;

		// Move down
		bsp_trace_box(
			&trace,
			end_pos,
			gs_vec3_add(end_pos, gs_v3(0, 0, -MG_MONSTER_STEP_HEIGHT)),
			monster->mins,
			monster->maxs,
			BSP_CONTENT_CONTENTS_SOLID | BSP_CONTENT_CONTENTS_MONSTERCLIP);
		if (trace.fraction == 1.0f || trace.end.z >= end_pos.z || trace.normal.z <= 0.7f)
			goto slide;

		monster->transform.position = trace.end;
		continue;

	slide:
		if (is_done) break; // if getting here from frac = 1.0 -> step_down

		// Can't step, slide along the plane, modify velocity for next loop
		monster->velocity = mg_clip_velocity(monster->velocity, hit_normal, 1.001f);
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