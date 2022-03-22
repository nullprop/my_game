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
#include "../graphics/model_manager.h"
#include "../graphics/renderer.h"
#include "../graphics/ui_manager.h"
#include "../util/camera.h"
#include "../util/config.h"
#include "../util/math.h"

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
				   .aspect_ratio = vid_width->value.f / vid_height->value.f,
				   .far_plane	 = 3000.0f,
				   .fov		 = mg_cvar("r_fov")->value.i,
				   .near_plane	 = 0.1f,
				   .proj_type	 = GS_PROJECTION_TYPE_PERSPECTIVE,
			   },
		   },
		.viewmodel_camera = {
			.aspect_ratio = vid_width->value.f / vid_height->value.f,
			.far_plane    = 200.0f,
			.fov	      = 60.0f,
			.near_plane   = 0,
			.proj_type    = GS_PROJECTION_TYPE_PERSPECTIVE,
		},
		.health		     = 100,
		.eye_pos	     = gs_v3(0, 0, MG_PLAYER_HEIGHT - MG_PLAYER_EYE_OFFSET),
		.mins		     = gs_v3(-MG_PLAYER_HALF_WIDTH, -MG_PLAYER_HALF_WIDTH, 0),
		.maxs		     = gs_v3(MG_PLAYER_HALF_WIDTH, MG_PLAYER_HALF_WIDTH, MG_PLAYER_HEIGHT),
		.viewmodel_transform = gs_vqs_default(),
		.last_ground_time    = 0,
	};

	// mg_model_t *model = mg_model_manager_find("models/Suzanne/glTF/suzanne.gltf");
	// player->viewmodel_handle = mg_renderer_create_renderable(*model, &player->viewmodel_transform);

	_mg_player_camera_update(player);

	return player;
}

void mg_player_free(mg_player_t *player)
{
	// mg_renderer_remove_renderable(player->viewmodel_handle);
	gs_free(player);
	player = NULL;
}

void mg_player_update(mg_player_t *player)
{
	if (g_ui_manager->show_cursor) return;

	gs_platform_t *platform = gs_subsystem(platform);
	float dt		= platform->time.delta;
	double pt		= gs_platform_elapsed_time();

	_mg_player_check_floor(player);
	_mg_player_get_input(player, dt);

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
		player->velocity = gs_vec3_add(player->velocity, gs_vec3_scale(MG_AXIS_DOWN, 800.0f * dt));

		// coyote
		if (player->wish_jump && !player->has_jumped && pt - player->last_ground_time <= MG_PLAYER_COYOTE_TIME * 1000)
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
	_mg_player_friction(player, dt);

	if (player->grounded)
	{
		if (player->crouched)
		{
			_mg_player_accelerate(player, dt, mg_lerp(MG_PLAYER_MOVE_SPEED, MG_PLAYER_CROUCH_MOVE_SPEED, player->crouch_fraction), MG_PLAYER_ACCEL, MG_PLAYER_MAX_VEL);
		}
		else
		{
			_mg_player_accelerate(player, dt, MG_PLAYER_MOVE_SPEED, MG_PLAYER_ACCEL, MG_PLAYER_MAX_VEL);
		}
	}
	else
	{
		_mg_player_accelerate(player, dt, MG_PLAYER_AIR_MOVE_SPEED, MG_PLAYER_AIR_ACCEL, MG_PLAYER_MAX_VEL);
	}

	// Move
	_mg_player_slidemove(player, dt);

	_mg_player_camera_update(player);

	// Check out of map bounds
	uint32_t leaf_index   = player->map->stats.current_leaf;
	int32_t cluster_index = player->map->leaves.data[leaf_index].cluster;
	if (cluster_index < 0)
	{
		gs_println(
			"WARN: player in invalid leaf, reset to last valid pos: [%f, %f, %f]",
			player->last_valid_pos.x,
			player->last_valid_pos.y,
			player->last_valid_pos.z);
		player->transform.position = player->last_valid_pos;
		player->velocity	   = gs_v3(0, 0, 0);
	}
}

void _mg_player_do_jump(mg_player_t *player)
{
	player->velocity.z = MG_PLAYER_JUMP_SPEED;
	player->grounded   = false;
	player->has_jumped = true;
	if (g_audio_manager != NULL)
		mg_audio_manager_play("sound/player/jump1.wav", 0.03f);
}

void _mg_player_check_floor(mg_player_t *player)
{
	bsp_trace_t trace = {.map = player->map};
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
		player->last_ground_time = gs_platform_elapsed_time();

		uint32_t leaf_index   = player->map->stats.current_leaf;
		int32_t cluster_index = player->map->leaves.data[leaf_index].cluster;
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

void _mg_player_get_input(mg_player_t *player, float delta_time)
{
	player->wish_move   = gs_v3(0, 0, 0);
	player->wish_jump   = false;
	player->wish_crouch = false;

	gs_vec2 dp = gs_vec2_scale(gs_platform_mouse_deltav(), mg_cvar("cl_sensitivity")->value.f * 0.022f);

	if (gs_platform_key_down(GS_KEYCODE_UP))
		dp.y -= 150.0f * delta_time;
	if (gs_platform_key_down(GS_KEYCODE_DOWN))
		dp.y += 150.0f * delta_time;
	if (gs_platform_key_down(GS_KEYCODE_RIGHT))
		dp.x += 150.0f * delta_time;
	if (gs_platform_key_down(GS_KEYCODE_LEFT))
		dp.x -= 150.0f * delta_time;

	// Rotate
	player->camera.pitch	   = gs_clamp(player->camera.pitch + dp.y, -90.0f, 90.0f);
	player->yaw		   = fmodf(player->yaw - dp.x, 360.0f);
	player->transform.rotation = gs_quat_angle_axis(gs_deg2rad(player->yaw), MG_AXIS_UP);

	if (gs_platform_key_down(GS_KEYCODE_W))
		player->wish_move = gs_vec3_add(player->wish_move, mg_get_forward(player->transform.rotation));
	if (gs_platform_key_down(GS_KEYCODE_S))
		player->wish_move = gs_vec3_add(player->wish_move, mg_get_backward(player->transform.rotation));
	if (gs_platform_key_down(GS_KEYCODE_D))
		player->wish_move = gs_vec3_add(player->wish_move, mg_get_right(player->transform.rotation));
	if (gs_platform_key_down(GS_KEYCODE_A))
		player->wish_move = gs_vec3_add(player->wish_move, mg_get_left(player->transform.rotation));

	player->wish_move.z = 0;
	player->wish_move   = gs_vec3_norm(player->wish_move);

	if (gs_platform_key_down(GS_KEYCODE_SPACE))
		player->wish_jump = true;
	if (gs_platform_key_down(GS_KEYCODE_LEFT_CONTROL))
		player->wish_crouch = true;
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

	player->viewmodel_transform = gs_vqs_absolute_transform(
		&(gs_vqs){
			.position = gs_v3(0, 15, -10),
			.rotation = gs_quat_angle_axis(gs_deg2rad(90), MG_AXIS_RIGHT),
			.scale	  = gs_v3(4.0f, 4.0f, 4.0f),
		},
		&player->camera.cam.transform);
}

void _mg_player_friction(mg_player_t *player, float delta_time)
{
	if (player->grounded == false) return;

	float vel2 = gs_vec3_len2(player->velocity);
	if (vel2 < GS_EPSILON)
	{
		player->velocity = gs_v3(0, 0, 0);
		return;
	}

	float vel      = sqrtf(vel2);
	float loss     = fmaxf(vel, MG_PLAYER_STOP_SPEED) * MG_PLAYER_FRICTION * delta_time;
	float fraction = fmaxf(0, vel - loss) / vel;

	player->velocity = gs_vec3_scale(player->velocity, fraction);
}

void _mg_player_accelerate(mg_player_t *player, float delta_time, float move_speed, float acceleration, float max_vel)
{
	// Velocity projected on to wished direction,
	// positive if in the same direction.
	float proj_vel = gs_vec3_dot(player->velocity, player->wish_move);

	// The max amount to change by,
	// won't accelerate past move_speed.
	float change = move_speed - proj_vel;
	if (change <= 0) return;

	// The actual acceleration
	float accel = acceleration * move_speed * delta_time;

	// Clamp to max change
	if (accel > change) accel = change;

	player->velocity = gs_vec3_add(player->velocity, gs_vec3_scale(player->wish_move, accel));

	// Clamp to max vel per axis
	player->velocity.x = fminf(MG_PLAYER_MAX_VEL, fmaxf(player->velocity.x, -MG_PLAYER_MAX_VEL));
	player->velocity.y = fminf(MG_PLAYER_MAX_VEL, fmaxf(player->velocity.y, -MG_PLAYER_MAX_VEL));
	player->velocity.z = fminf(MG_PLAYER_MAX_VEL, fmaxf(player->velocity.z, -MG_PLAYER_MAX_VEL));
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

	bsp_trace_t trace = {.map = player->map};
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

void _mg_player_slidemove(mg_player_t *player, float delta_time)
{
	uint16_t current_iter = 0;
	uint16_t max_iter     = 10;
	gs_vec3 start;
	gs_vec3 end;
	bsp_trace_t trace = {.map = player->map};
	float32_t prev_frac;
	gs_vec3 prev_normal;

	while (delta_time > 0)
	{
		if (gs_vec3_len2(player->velocity) == 0)
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

		start = player->transform.position;
		end   = gs_vec3_add(start, gs_vec3_scale(player->velocity, delta_time));

		bsp_trace_box(
			&trace,
			start,
			end,
			player->mins,
			player->maxs,
			BSP_CONTENT_CONTENTS_SOLID | BSP_CONTENT_CONTENTS_PLAYERCLIP);

		if (trace.start_solid)
		{
			// Stuck in a solid, try to get out
			if (trace.all_solid)
			{
				_mg_player_unstuck(player);
			}
			else
			{
				gs_println(
					"WARN: player start solid but not stuck: [%f, %f, %f] -> [%f, %f, %f]",
					player->transform.position.x,
					player->transform.position.y,
					player->transform.position.z,
					trace.end.x,
					trace.end.y,
					trace.end.z);
				player->transform.position = trace.end;
				player->velocity	   = gs_v3(0, 0, 0);
			}
			break;
		}

		if (trace.fraction > 0)
		{
			// Move as far as we can
			player->transform.position = trace.end;
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
		// 	Hugging a wall stops player from going up stairs.
		// 	The forward trace will collide with the wall if velocity
		// 	is angled towards it.
		// 	Potential solution: If forward trace collides with anything,
		// 	project velocity to normal and try again?

		// Check above
		bsp_trace_box(
			&trace,
			end_pos,
			gs_vec3_add(end_pos, gs_v3(0, 0, MG_PLAYER_STEP_HEIGHT)),
			player->mins,
			player->maxs,
			BSP_CONTENT_CONTENTS_SOLID | BSP_CONTENT_CONTENTS_PLAYERCLIP);
		if (trace.fraction <= 0)
			goto step_down;

		// Check forward
		gs_vec3 horizontal_vel = player->velocity;
		horizontal_vel.z       = 0;
		horizontal_vel	       = gs_vec3_scale(horizontal_vel, delta_time);
		// Player can be epsilon from an edge on either axis,
		// and will need to go epsilon over it for downwards trace to hit anything.
		// Scaling both axes to a min of 2 * epsilon (if not 0) allows player to
		// step up stairs even if moving at a very shallow angle towards them.
		// While this can make the player move a very tiny distance further,
		// it's imperceptible in testing and preferred to getting stuck on a stair.
		if (fabs(horizontal_vel.x) > GS_EPSILON && fabs(horizontal_vel.x) < BSP_TRACE_EPSILON * 2.0f)
			horizontal_vel.x = (horizontal_vel.x / fabs(horizontal_vel.x)) * BSP_TRACE_EPSILON * 2.0f;
		if (fabs(horizontal_vel.y) > GS_EPSILON && fabs(horizontal_vel.y) < BSP_TRACE_EPSILON * 2.0f)
			horizontal_vel.y = (horizontal_vel.y / fabs(horizontal_vel.y)) * BSP_TRACE_EPSILON * 2.0f;

		bsp_trace_box(
			&trace,
			trace.end,
			gs_vec3_add(trace.end, horizontal_vel),
			player->mins,
			player->maxs,
			BSP_CONTENT_CONTENTS_SOLID | BSP_CONTENT_CONTENTS_PLAYERCLIP);
		if (trace.fraction <= 0)
			goto step_down;

		float forward_frac = trace.fraction;

		// Move down
		bsp_trace_box(
			&trace,
			trace.end,
			gs_vec3_add(trace.end, gs_v3(0, 0, -MG_PLAYER_STEP_HEIGHT)),
			player->mins,
			player->maxs,
			BSP_CONTENT_CONTENTS_SOLID | BSP_CONTENT_CONTENTS_PLAYERCLIP);
		if (trace.fraction == 1.0f || trace.end.z <= end_pos.z || trace.normal.z <= 0.7f)
			goto step_down;

		player->transform.position = trace.end;
		delta_time -= delta_time * forward_frac;
		continue;

	step_down:
		// We're already going 'forward' as much as we can,
		// try to stick to the floor if grounded before.
		if (!player->grounded)
			goto slide;

		// Move down
		bsp_trace_box(
			&trace,
			end_pos,
			gs_vec3_add(end_pos, gs_v3(0, 0, -MG_PLAYER_STEP_HEIGHT)),
			player->mins,
			player->maxs,
			BSP_CONTENT_CONTENTS_SOLID | BSP_CONTENT_CONTENTS_PLAYERCLIP);
		if (trace.fraction == 1.0f || trace.end.z >= end_pos.z || trace.normal.z <= 0.7f)
			goto slide;

		player->transform.position = trace.end;
		continue;

	slide:
		if (is_done) break; // if getting here from frac = 1.0 -> step_down

		// Can't step, slide along the plane, modify velocity for next loop
		player->velocity = mg_clip_velocity(player->velocity, hit_normal, 1.001f);
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
	bsp_trace_t trace  = {.map = player->map};

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

			gs_println(
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
				gs_println(
					"WARN: player stuck in solid at [%f, %f, %f], could not unstuck.",
					player->transform.position.x,
					player->transform.position.y,
					player->transform.position.z);
				break;
			}
		}
	}
}