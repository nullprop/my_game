/*================================================================
	* fx/rocket_trail.c
	*
	* Copyright (c) 2022 Lauri Räsänen
	* ================================

	...
=================================================================*/

#include "rocket_trail.h"
#include "../entities/entity_manager.h"

mg_rocket_trail_t *mg_rocket_trail_new(gs_vqs *rocket_transform)
{
	mg_rocket_trail_t *trail = gs_malloc_init(mg_rocket_trail_t);
	// TODO: flame
	// TODO: smoke
	// TODO: play animations
	mg_entity_manager_add_entity(&trail->mdl_ent_fire.ent, mg_rocket_trail_update, mg_rocket_trail_free);
	return trail;
}

void mg_rocket_trail_free(mg_rocket_trail_t *trail)
{
	// TODO: free all
	gs_free(trail);
	trail = NULL;
}

void mg_rocket_trail_update(mg_rocket_trail_t *trail, double dt)
{
}

void mg_rocket_trail_remove(mg_rocket_trail_t *trail)
{
	mg_entity_manager_remove_entity(trail->mdl_ent_fire.ent.id);
	mg_rocket_trail_free(trail);
}