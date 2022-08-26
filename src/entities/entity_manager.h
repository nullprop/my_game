/*================================================================
	* entities/entity_manager.h
	*
	* Copyright (c) 2022 Lauri Räsänen
	* ================================

	...
=================================================================*/

#ifndef MG_ENTITY_MANAGER_H
#define MG_ENTITY_MANAGER_H

#include <gs/gs.h>

#include "entity.h"

typedef struct mg_entity_funcs_t
{
	mg_entity_t *entity;
	void (*update_func)(void *, double);
	void (*free_func)(void *);
} mg_entity_funcs_t;

typedef struct mg_entity_manager_t
{
	gs_slot_array(mg_entity_funcs_t) ent_funcs;
} mg_entity_manager_t;

void mg_entity_manager_init();
void mg_entity_manager_free();
void mg_entity_manager_update();
uint32_t mg_entity_manager_add_entity(mg_entity_t *entity, void (*update_func)(void *, double), void (*free_func)(void *));
void mg_entity_manager_remove_entity(const uint32_t id);

extern mg_entity_manager_t *g_entity_manager;

#endif // MG_ENTITY_MANAGER_H