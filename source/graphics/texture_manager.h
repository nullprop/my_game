/*================================================================
    * graphics/texture_manager.h
    *
    * Copyright (c) 2021 Lauri Räsänen
    * ================================

    ...
=================================================================*/

#ifndef MG_TEXTURE_MANAGER_H
#define MG_TEXTURE_MANAGER_H

#include <gs/gs.h>

typedef struct mg_texture_t
{
    char *filename;
    gs_asset_texture_t *asset;
} mg_texture_t;

typedef struct mg_texture_manager_t
{
    gs_dyn_array(mg_texture_t) textures;
} mg_texture_manager_t;

void mg_texture_manager_init();
void mg_texture_manager_free();
gs_asset_texture_t *mg_texture_manager_get(char *path);
bool32_t _mg_texture_manager_load(char *path, gs_asset_texture_t *asset);
gs_asset_texture_t *_mg_texture_manager_find(char *filename);

extern mg_texture_manager_t *g_texture_manager;

#endif // MG_TEXTURE_MANAGER_H