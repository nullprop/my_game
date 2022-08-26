/*================================================================
	* fx/rocket_trail.h
	*
	* Copyright (c) 2022 Lauri Räsänen
	* ================================

	...
=================================================================*/

#ifndef MG_ROCKET_TRAIL_H
#define MG_ROCKET_TRAIL_H

#include <gs/gs.h>

#include "../entities/entity.h"

typedef struct mg_rocket_trail_t
{
	gs_vqs *rocket_transform;
	mg_model_entity_t mdl_ent_fire;
	mg_model_entity_t mdl_ent_smoke;
} mg_rocket_trail_t;

mg_rocket_trail_t *mg_rocket_trail_new(gs_vqs *rocket_transform);
void mg_rocket_trail_free(mg_rocket_trail_t *trail);
void mg_rocket_trail_update(mg_rocket_trail_t *trail, double dt);
void mg_rocket_trail_remove(mg_rocket_trail_t *trail);

#endif // MG_ROCKET_TRAIL_H