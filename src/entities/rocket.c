/*================================================================
	* entities/rocket.c
	*
	* Copyright (c) 2022 Lauri Räsänen
	* ================================

	...
=================================================================*/

#include "rocket.h"
#include "../bsp/bsp_trace.h"
#include "../game/console.h"
#include "../game/time_manager.h"
#include "../util/transform.h"
#include "entity_manager.h"

mg_rocket_t *mg_rocket_new(gs_vqs transform)
{
	mg_rocket_t *rocket = gs_malloc_init(mg_rocket_t);
	gs_assert(mg_model_ent_init(&rocket->mdl_ent, transform, "projectiles/rocket.md3", "basic"));
	rocket->mdl_ent.ent.velocity = gs_vec3_scale(mg_get_forward(transform.rotation), MG_ROCKET_SPEED);
	rocket->life_time	     = MG_ROCKET_LIFE;
	rocket->start_time	     = g_time_manager->time;
	rocket->hidden		     = true;
	rocket->trail		     = mg_rocket_trail_new(&rocket->mdl_ent.ent.transform);
	mg_renderer_set_hidden(rocket->mdl_ent.renderable_id, true);
	mg_entity_manager_add_entity(rocket, mg_rocket_update, mg_rocket_free);
	return rocket;
}

void mg_rocket_free(mg_rocket_t *rocket)
{
	if (rocket->trail != NULL)
	{
		mg_rocket_trail_free(rocket->trail);
	}

	mg_model_ent_free(&rocket->mdl_ent);
	gs_free(rocket);
	rocket = NULL;
}

void mg_rocket_update(mg_rocket_t *rocket, double dt)
{
	rocket->life_time -= dt;
	if (rocket->life_time <= 0)
	{
		_mg_rocket_explode(rocket);
		return;
	}

	double time = g_time_manager->time;
	if (rocket->hidden && time - rocket->start_time >= MG_ROCKET_HIDE_TIME)
	{
		rocket->hidden = false;
		mg_renderer_set_hidden(rocket->mdl_ent.renderable_id, false);
	}

	gs_vec3 current_pos = rocket->mdl_ent.ent.transform.position;
	gs_vec3 new_pos	    = gs_vec3_add(
		    current_pos,
		    gs_vec3_scale(rocket->mdl_ent.ent.velocity, dt));

	bsp_trace_t trace = {0};
	trace.map	  = g_game_manager->map;
	bsp_trace_ray(&trace, current_pos, new_pos, BSP_CONTENT_CONTENTS_SOLID);

	if (trace.start_solid)
	{
		_mg_rocket_remove(rocket);
		return;
	}

	if (trace.fraction < 1.0)
	{
		// Pull away from surface a bit so explosion can see entities better
		gs_vec3 pull			       = gs_vec3_scale(trace.normal, 8.0f);
		rocket->mdl_ent.ent.transform.position = gs_vec3_add(
			trace.end,
			pull);
		_mg_rocket_explode(rocket);
		return;
	}

	// TODO: trace against entities

	rocket->mdl_ent.ent.transform.position = new_pos;

	// TODO: travel sound at pos
}

void _mg_rocket_remove(mg_rocket_t *rocket)
{
	mg_rocket_trail_remove(rocket->trail);
	mg_entity_manager_remove_entity(rocket->mdl_ent.ent.id);
	mg_rocket_free(rocket);
}

void _mg_rocket_explode(mg_rocket_t *rocket)
{
	// TODO: sphere iter nearby ents + trace + damage
	// TODO: explosion sound at pos
	// TODO: explosion fx
	_mg_rocket_remove(rocket);
}
