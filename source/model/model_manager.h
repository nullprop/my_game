/*================================================================
    * model/model_manager.h
    *
    * Copyright (c) 2021 Lauri Räsänen
    * ================================

    ...
=================================================================*/

#ifndef MG_MODEL_MANAGER_H
#define MG_MODEL_MANAGER_H

#include <gs/gs.h>

#include <assimp/scene.h>

typedef struct mg_model_data_t
{
    gs_dyn_array(gs_vec3) vertices;
    gs_dyn_array(uint16_t) indices;
} mg_model_data_t;

typedef struct mg_model_t
{
    char *filename;
    mg_model_data_t data;
} mg_model_t;

typedef struct mg_model_manager_t
{
    gs_dyn_array(mg_model_t) models;
} mg_model_manager_t;

void mg_model_manager_init();
void mg_model_manager_free();
mg_model_t *mg_model_manager_find(char *filename);
void _mg_model_manager_load(char *filename);

extern mg_model_manager_t *g_model_manager;

#endif // MG_MODEL_MANAGER_H