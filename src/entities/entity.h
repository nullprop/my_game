/*================================================================
	* entities/entity.h
	*
	* Copyright (c) 2022 Lauri Räsänen
	* ================================

	Common functions for entities.
	BSP movement, acceleration, etc.
=================================================================*/

#ifndef MG_ENTITY_H
#define MG_ENTITY_H

#include <gs/gs.h>

#include "../bsp/bsp_trace.h"
#include "../game/game_manager.h"

/**
 * Apply friction to entity.
 */
static inline void mg_ent_friction(gs_vec3 *velocity, const float stop_speed, const float friction, const float delta_time)
{
	float vel2 = gs_vec3_len2(*velocity);
	if (vel2 < GS_EPSILON)
	{
		*velocity = gs_v3(0, 0, 0);
		return;
	}

	float vel  = sqrtf(vel2);
	float loss = fmaxf(vel, stop_speed) * friction * delta_time;
	float frac = fmaxf(0, vel - loss) / vel;

	*velocity = gs_vec3_scale(*velocity, frac);
}

/**
 * Accelerate entity with wished move direction.
 */
static inline void mg_ent_accelerate(gs_vec3 *velocity, const gs_vec3 wish_move, const float move_speed, const float acceleration, const float max_vel, const float delta_time)
{
	// Velocity projected on to wished direction,
	// positive if in the same direction.
	float proj_vel = gs_vec3_dot(*velocity, wish_move);

	// The max amount to change by,
	// won't accelerate past move_speed.
	float change = move_speed - proj_vel;
	if (change <= 0) return;

	// The actual acceleration
	float accel = acceleration * move_speed * delta_time;

	// Clamp to max change
	if (accel > change) accel = change;

	*velocity = gs_vec3_add(*velocity, gs_vec3_scale(wish_move, accel));

	// Clamp to max vel per axis
	velocity->x = fminf(max_vel, fmaxf(velocity->x, -max_vel));
	velocity->y = fminf(max_vel, fmaxf(velocity->y, -max_vel));
	velocity->z = fminf(max_vel, fmaxf(velocity->z, -max_vel));
}

/**
 * Slide and step through the BSP.
 * Will try to remain on ground when stepping down if grounded=true.
 * Returns false if stuck.
 */
static inline bool mg_ent_slidemove(
	gs_vqs *transform,
	gs_vec3 *velocity,
	const gs_vec3 mins,
	const gs_vec3 maxs,
	const float max_step_height,
	const bool grounded,
	const int32_t content_mask,
	const float delta_time)
{
	uint16_t current_iter = 0;
	uint16_t max_iter     = 10;
	gs_vec3 start;
	gs_vec3 end;
	bsp_trace_t trace = {.map = g_game_manager->map};
	float32_t prev_frac;
	gs_vec3 prev_normal;
	float dt = delta_time;

	while (dt > 0)
	{
		if (gs_vec3_len2(*velocity) == 0)
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

		start = transform->position;
		end   = gs_vec3_add(start, gs_vec3_scale(*velocity, dt));

		bsp_trace_box(
			&trace,
			start,
			end,
			mins,
			maxs,
			content_mask);

		if (trace.start_solid)
		{
			// Stuck in a solid, try to get out
			if (trace.all_solid)
			{
				// Proper stuck
				return false;
			}
			else
			{
				mg_println(
					"WARN: slidemove start solid but not stuck: [%f, %f, %f] -> [%f, %f, %f]",
					transform->position.x,
					transform->position.y,
					transform->position.z,
					trace.end.x,
					trace.end.y,
					trace.end.z);
				transform->position = trace.end;
				*velocity	    = gs_v3(0, 0, 0);
			}
			break;
		}

		if (trace.fraction > 0)
		{
			// Move as far as we can
			transform->position = trace.end;
		}

		bool is_done	= false;
		gs_vec3 end_pos = trace.end;

		if (trace.fraction == 1.0f)
		{
			// Moved all the way
			dt	= 0;
			is_done = true;
			goto step_down;
		}

		dt -= dt * trace.fraction;

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
			gs_vec3_add(end_pos, gs_v3(0, 0, max_step_height)),
			mins,
			maxs,
			content_mask);
		if (trace.fraction <= 0)
			goto step_down;

		// Check forward
		gs_vec3 horizontal_vel = *velocity;
		horizontal_vel.z       = 0;
		horizontal_vel	       = gs_vec3_scale(horizontal_vel, dt);
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
			mins,
			maxs,
			content_mask);
		if (trace.fraction <= 0)
			goto step_down;

		float forward_frac = trace.fraction;

		// Move down
		bsp_trace_box(
			&trace,
			trace.end,
			gs_vec3_add(trace.end, gs_v3(0, 0, -max_step_height)),
			mins,
			maxs,
			content_mask);
		if (trace.fraction == 1.0f || trace.end.z <= end_pos.z || trace.normal.z <= 0.7f)
			goto step_down;

		transform->position = trace.end;
		dt -= dt * forward_frac;
		continue;

	step_down:
		// We're already going 'forward' as much as we can,
		// try to stick to the floor if grounded before.
		if (!grounded)
			goto slide;

		// Move down
		bsp_trace_box(
			&trace,
			end_pos,
			gs_vec3_add(end_pos, gs_v3(0, 0, -max_step_height)),
			mins,
			maxs,
			content_mask);
		if (trace.fraction == 1.0f || trace.end.z >= end_pos.z || trace.normal.z <= 0.7f)
			goto slide;

		transform->position = trace.end;
		continue;

	slide:
		if (is_done) break; // if getting here from frac = 1.0 -> step_down

		// Can't step, slide along the plane, modify velocity for next loop
		*velocity = mg_clip_velocity(*velocity, hit_normal, 1.001f);
	}

	return true;
}

#endif // MG_ENTITY_H