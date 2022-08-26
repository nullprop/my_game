/*================================================================
	* entities/entity_manager.c
	*
	* Copyright (c) 2022 Lauri Räsänen
	* ================================

	...
=================================================================*/

#include "entity_manager.h"
#include "../game/time_manager.h"

mg_entity_manager_t *g_entity_manager;

void mg_entity_manager_init()
{
	g_entity_manager	    = gs_malloc_init(mg_entity_manager_t);
	g_entity_manager->ent_funcs = gs_slot_array_new(mg_entity_func_wrapper_t);
}

void mg_entity_manager_free()
{
	for (
		gs_slot_array_iter it = gs_slot_array_iter_new(g_entity_manager->ent_funcs);
		gs_slot_array_iter_valid(g_entity_manager->ent_funcs, it);
		gs_slot_array_iter_advance(g_entity_manager->ent_funcs, it))
	{
		mg_entity_funcs_t ent = gs_slot_array_iter_get(g_entity_manager->ent_funcs, it);
		if (ent.free_func != NULL)
		{
			ent.free_func(ent.entity);
		}
	}
	gs_slot_array_free(g_entity_manager->ent_funcs);
}

void mg_entity_manager_update()
{
	double dt = g_time_manager->delta;
	for (
		gs_slot_array_iter it = gs_slot_array_iter_new(g_entity_manager->ent_funcs);
		gs_slot_array_iter_valid(g_entity_manager->ent_funcs, it);
		gs_slot_array_iter_advance(g_entity_manager->ent_funcs, it))
	{
		mg_entity_funcs_t ent = gs_slot_array_iter_get(g_entity_manager->ent_funcs, it);
		if (ent.update_func != NULL)
		{
			ent.update_func(ent.entity, dt);
		}
	}
}

uint32_t mg_entity_manager_add_entity(mg_entity_t *entity, void (*update_func)(void *, double), void (*free_func)(void *))
{
	mg_entity_funcs_t ent_funcs = {
		.entity	     = entity,
		.update_func = update_func,
		.free_func   = free_func,
	};
	uint32_t id	     = gs_slot_array_insert(g_entity_manager->ent_funcs, ent_funcs);
	mg_entity_funcs_t *f = gs_slot_array_getp(g_entity_manager->ent_funcs, id);
	f->entity->id	     = id;
	return id;
}

void mg_entity_manager_remove_entity(const uint32_t id)
{
	if (gs_slot_array_handle_valid(g_entity_manager->ent_funcs, id))
	{
		gs_slot_array_erase(g_entity_manager->ent_funcs, id);
	}
}
