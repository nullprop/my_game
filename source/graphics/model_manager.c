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
    _mg_model_manager_load("models/test.gltf", NULL);
    _mg_model_manager_load("models/Suzanne/glTF/suzanne.gltf", "models/Suzanne/glTF/Suzanne_BaseColor.png");
    _mg_model_manager_load("models/Sponza/glTF/Sponza.gltf", NULL);
}

void mg_model_manager_free()
{
    for (size_t i = 0; i < gs_dyn_array_size(g_model_manager->models); i++)
    {
        gs_free(g_model_manager->models[i].texture);
        g_model_manager->models[i].texture = NULL;
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

    gs_println("WARN: mg_model_manager_find invalid model %s", filename);
    return NULL;
}

void _mg_model_manager_load(char *filename, char *texture)
{
    if (!gs_util_file_exists(filename))
    {
        gs_println("ERR: _mg_model_manager_load file not found %s", filename);
        return;
    }

    gs_gfxt_mesh_import_options_t options = {
        .layout = (gs_gfxt_mesh_layout_t[]){
            {.type = GS_GFXT_MESH_ATTRIBUTE_TYPE_POSITION},
            {.type = GS_GFXT_MESH_ATTRIBUTE_TYPE_TEXCOORD},
        },
        .layout_size = 2 * sizeof(gs_gfxt_mesh_layout_t),
        .index_buffer_element_size = sizeof(uint32_t),
    };

    mg_model_t model = {
        .filename = filename,
        .data = gs_gfxt_mesh_create_from_file(filename, &options),
        .texture = NULL,
    };

    // TODO: remove in favor of gs_gfxt materials
    if (texture != NULL)
    {
        model.texture = gs_malloc_init(gs_asset_texture_t);
        gs_asset_texture_load_from_file(
            texture,
            model.texture,
            &(gs_graphics_texture_desc_t){
                .format = GS_GRAPHICS_TEXTURE_FORMAT_RGBA8,
                .min_filter = GS_GRAPHICS_TEXTURE_FILTER_NEAREST,
                .mag_filter = GS_GRAPHICS_TEXTURE_FILTER_NEAREST,
            },
            false,
            false);
    }

    gs_dyn_array_push(g_model_manager->models, model);

    gs_println("Model: Loaded %s", filename);
}