/*================================================================
    * model/model_manager.c
    *
    * Copyright (c) 2021 Lauri Räsänen
    * ================================

    ...
=================================================================*/

#include "model_manager.h"

mg_model_manager_t *g_model_manager;

void mg_model_manager_init()
{
    // Allocate
    g_model_manager = gs_malloc_init(mg_model_manager_t);
    g_model_manager->models = gs_dyn_array_new(mg_model_t);

    // Test
    _mg_model_manager_load("models/test.gltf");
    _mg_model_manager_load("models/Suzanne/glTF/suzanne.gltf");
    _mg_model_manager_load("models/Sponza/glTF/Sponza.gltf");
}

void mg_model_manager_free()
{
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

    gs_println("WARN: mg_model_manager_find invalid model %s", filename);
    return NULL;
}

void _mg_model_manager_load(char *filename)
{
    if (!gs_util_file_exists(filename))
    {
        gs_println("ERR: _mg_model_manager_load file not found %s", filename);
        return;
    }

    gs_gfxt_mesh_import_options_t options = {
        .layout = (gs_gfxt_mesh_layout_t[]){
            {.type = GS_GFXT_MESH_ATTRIBUTE_TYPE_POSITION},
        },
        .layout_size = 1 * sizeof(gs_gfxt_mesh_layout_t),
        .index_buffer_element_size = sizeof(uint32_t),
    };

    mg_model_t model = {
        .filename = filename,
        .data = gs_gfxt_mesh_create_from_file(filename, &options),
    };

    gs_dyn_array_push(g_model_manager->models, model);

    gs_println("Model: Loaded %s", filename);
}