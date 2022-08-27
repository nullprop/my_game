/*================================================================
	* fx/rocket_trail.c
	*
	* Copyright (c) 2022 Lauri Räsänen
	* ================================

	...
=================================================================*/

#include "rocket_trail.h"
#include "../entities/entity_manager.h"
#include "../game/time_manager.h"

mg_rocket_trail_t *mg_rocket_trail_new(gs_vqs *rocket_transform)
{
	mg_rocket_trail_t *trail = gs_malloc_init(mg_rocket_trail_t);
	gs_assert(mg_model_ent_init(&trail->mdl_ent_fire, *rocket_transform, "fx/rocket_flame.md3", "basic"));
	gs_assert(mg_model_ent_init(&trail->mdl_ent_smoke, *rocket_transform, "fx/rocket_smoke.md3", "basic"));
	mg_entity_manager_add_entity(&trail->mdl_ent_fire.ent, mg_rocket_trail_update, mg_rocket_trail_free);
	trail->rocket_transform = rocket_transform;
	trail->attached		= true;
	mg_renderer_play_animation(trail->mdl_ent_fire.renderable_id, "BURN");
	mg_renderer_play_animation(trail->mdl_ent_smoke.renderable_id, "BURN");
	return trail;
}

void mg_rocket_trail_free(mg_rocket_trail_t *trail)
{
	mg_model_ent_free(&trail->mdl_ent_fire);
	mg_model_ent_free(&trail->mdl_ent_smoke);
	gs_free(trail);
	trail = NULL;
}

void mg_rocket_trail_update(mg_rocket_trail_t *trail, double dt)
{
	if (!trail->attached)
	{
		double frac = (g_time_manager->time - trail->detach_time) / MG_ROCKET_TRAIL_FADE_TIME;
		if (frac >= 1.0)
		{
			mg_rocket_trail_remove(trail);
			return;
		}
		float alpha = gs_max(0, 1.0 - frac);
		// TODO: set renderables override alpha
		return;
	}

	trail->mdl_ent_fire.ent.transform = gs_vqs_absolute_transform(
		&(gs_vqs){
			.position = gs_v3(0.0f, 0.0f, 0.0f),
			.rotation = gs_quat_default(),
			.scale	  = gs_v3(1.0f, 1.0f, 1.0f),
		},
		trail->rocket_transform);
	trail->mdl_ent_smoke.ent.transform = gs_vqs_absolute_transform(
		&(gs_vqs){
			.position = gs_v3(0.0f, 0.0f, 0.0f),
			.rotation = gs_quat_default(),
			.scale	  = gs_v3(1.0f, 1.0f, 1.0f),
		},
		trail->rocket_transform);
}

void mg_rocket_trail_remove(mg_rocket_trail_t *trail)
{
	mg_entity_manager_remove_entity(trail->mdl_ent_fire.ent.id);
	mg_rocket_trail_free(trail);
}

void mg_rocket_trail_detach(mg_rocket_trail_t *trail)
{
	trail->attached		= false;
	trail->rocket_transform = NULL;
	trail->detach_time	= g_time_manager->time;
}