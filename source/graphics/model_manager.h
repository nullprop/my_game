/*================================================================
	* graphics/model_manager.h
	*
	* Copyright (c) 2021 Lauri Räsänen
	* ================================

	...
=================================================================*/

#ifndef MG_MODEL_MANAGER_H
#define MG_MODEL_MANAGER_H

#include <gs/gs.h>

#include "model.h"

typedef struct mg_model_t
{
	char *filename;
	char *shader;
	md3_t *data;
} mg_model_t;

typedef struct mg_model_manager_t
{
	gs_dyn_array(mg_model_t) models;
} mg_model_manager_t;

void mg_model_manager_init();
void mg_model_manager_free();
mg_model_t *mg_model_manager_find(char *filename);
void _mg_model_manager_load(char *filename, char *shader);

extern mg_model_manager_t *g_model_manager;

#endif // MG_MODEL_MANAGER_H