/*================================================================
	* fx/rocket_trail.h
	*
	* Copyright (c) 2022 Lauri Räsänen
	* ================================

	...
=================================================================*/

#ifndef MG_ROCKET_TRAIL_H
#define MG_ROCKET_TRAIL_H

#define MG_ROCKET_TRAIL_FADE_TIME 0.1

#include <gs/gs.h>

#include "../entities/entity.h"

typedef struct mg_rocket_trail_t
{
	mg_model_entity_t mdl_ent_fire; // first for casting from entity manager
	mg_model_entity_t mdl_ent_smoke;
	gs_vqs *rocket_transform;
	bool attached;
	double detach_time;
} mg_rocket_trail_t;

mg_rocket_trail_t *mg_rocket_trail_new(gs_vqs *rocket_transform);
void mg_rocket_trail_free(mg_rocket_trail_t *trail);
void mg_rocket_trail_update(mg_rocket_trail_t *trail, double dt);
void mg_rocket_trail_remove(mg_rocket_trail_t *trail);
void mg_rocket_trail_detach(mg_rocket_trail_t *trail);

#endif // MG_ROCKET_TRAIL_H