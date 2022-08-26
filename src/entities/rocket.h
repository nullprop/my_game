/*================================================================
	* entities/rocket.h
	*
	* Copyright (c) 2022 Lauri Räsänen
	* ================================

	...
=================================================================*/

#ifndef MG_ROCKET_H
#define MG_ROCKET_H

#include "entity.h"

#define MG_ROCKET_SPEED 800.0
#define MG_ROCKET_LIFE	10.0
#define MG_ROCKET_HIDE_TIME 0.025

typedef struct mg_rocket_t
{
	mg_model_entity_t mdl_ent;
	bool hidden;
	double life_time;
	double start_time;
} mg_rocket_t;

mg_rocket_t *mg_rocket_new(gs_vqs transform);
void mg_rocket_free(mg_rocket_t *rocket);
void mg_rocket_update(mg_rocket_t *rocket, double dt);
void _mg_rocket_remove(mg_rocket_t *rocket);
void _mg_rocket_explode(mg_rocket_t *rocket);

#endif // MG_ROCKET_H