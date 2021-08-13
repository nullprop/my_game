/*================================================================
    * model/model_manager.c
    *
    * Copyright (c) 2021 Lauri Räsänen
    * ================================

    ...
=================================================================*/

#include <assimp/cimport.h>
#include <assimp/postprocess.h>

#include "model_manager.h"

mg_model_manager_t *g_model_manager;

void mg_model_manager_init()
{
    // Allocate
    g_model_manager = gs_malloc_init(mg_model_manager_t);
    g_model_manager->models = gs_dyn_array_new(mg_model_t);

    // Test
    _mg_model_manager_load("models/test.fbx");
}

void mg_model_manager_free()
{
    for (size_t i = 0; i < gs_dyn_array_size(g_model_manager->models); i++)
    {
        gs_free(g_model_manager->models[i].data.vertices);
        gs_free(g_model_manager->models[i].data.indices);
        g_model_manager->models[i].data.vertices = NULL;
        g_model_manager->models[i].data.indices = NULL;
    }

    gs_dyn_array_free(g_model_manager->models);

    gs_free(g_model_manager);
    g_model_manager = NULL;
}

mg_model_t *mg_model_manager_find(char *filename)
{
    for (size_t i = 0; i < gs_dyn_array_size(g_model_manager->models); i++)
    {
        if (strcmp(filename, g_model_manager->models[i].filename) == 0)
        {
            return &g_model_manager->models[i];
        }
    }

    return NULL;
}

void _mg_model_manager_load(char *filename)
{
    const struct aiScene *scene = aiImportFile(
        filename,
        aiProcess_CalcTangentSpace |
            aiProcess_Triangulate |
            aiProcess_JoinIdenticalVertices |
            aiProcess_SortByPType);

    // TODO: grab vertices etc. from scene
    mg_model_t model = (mg_model_t){
        .filename = filename,
        .data = &(mg_model_data_t){
            .vertices = gs_dyn_array_new(gs_vec3),
            .indices = gs_dyn_array_new(uint16_t),
        },
    };

    aiReleaseImport(scene);

    gs_dyn_array_push(g_model_manager->models, model);
}