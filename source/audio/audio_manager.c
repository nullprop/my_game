/*================================================================
    * audio/audio_manager.c
    *
    * Copyright (c) 2021 Lauri Räsänen
    * ================================

    ...
=================================================================*/

#include "audio_manager.h"
#include "../util/string.h"

mg_audio_manager_t *g_audio_manager;

void mg_audio_manager_init()
{
    g_audio_manager = gs_malloc_init(mg_audio_manager_t);
    g_audio_manager->assets = gs_dyn_array_new(mg_audio_asset_t);

    // TODO config
    g_audio_manager->master_vol = 0.1f;
    g_audio_manager->mixer_vol[MG_AUDIO_TYPE_EFFECT] = 1.0f;
    g_audio_manager->mixer_vol[MG_AUDIO_TYPE_MUSIC] = 0.8f;
    g_audio_manager->mixer_vol[MG_AUDIO_TYPE_AMBIENT] = 0.6f;

    // Player
    _mg_audio_manager_load("sound/player/jump1.wav", MG_AUDIO_TYPE_EFFECT, false, false);
}

void mg_audio_manager_free()
{
    for (size_t i = 0; i < gs_dyn_array_size(g_audio_manager->assets); i++)
    {
        gs_audio_stop(g_audio_manager->assets[i].instance);
    }

    gs_dyn_array_free(g_audio_manager->assets);
    gs_free(g_audio_manager);
    g_audio_manager = NULL;
}

void mg_audio_manager_play(char *name)
{
    mg_audio_asset_t *asset = _mg_audio_manager_find(name);
    if (asset == NULL)
    {
        gs_println("WARN: mg_audio_manager_play invalid audio %s", name);
        return;
    }

    if (asset->loop)
    {
        gs_audio_play(asset->instance);
    }
    else
    {
        gs_audio_play_source(asset->source, g_audio_manager->master_vol * g_audio_manager->mixer_vol[asset->type]);
    }
}

void mg_audio_manager_stop(char *name)
{
    mg_audio_asset_t *asset = _mg_audio_manager_find(name);
    if (asset == NULL)
    {
        gs_println("WARN: mg_audio_manager_stop invalid audio %s", name);
        return;
    }

    gs_audio_stop(asset->instance);
}

void mg_audio_manager_pause(char *name)
{
    mg_audio_asset_t *asset = _mg_audio_manager_find(name);
    if (asset == NULL)
    {
        gs_println("WARN: mg_audio_manager_pause invalid audio %s", name);
        return;
    }

    gs_audio_pause(asset->instance);
}

void mg_audio_manager_restart(char *name)
{
    mg_audio_asset_t *asset = _mg_audio_manager_find(name);
    if (asset == NULL)
    {
        gs_println("WARN: mg_audio_manager_restart invalid audio %s", name);
        return;
    }

    gs_audio_restart(asset->instance);
}

bool mg_audio_manager_is_playing(char *name)
{
    mg_audio_asset_t *asset = _mg_audio_manager_find(name);
    if (asset == NULL)
    {
        gs_println("WARN: mg_audio_manager_is_playing invalid audio %s", name);
        return;
    }

    return gs_audio_is_playing(asset->instance);
}

void _mg_audio_manager_load(char *filename, mg_audio_type type, bool loop, bool persistent)
{
    mg_audio_asset_t asset = (mg_audio_asset_t){
        .source = gs_audio_load_from_file(filename),
        .type = type,
        .loop = loop,
        .persistent = persistent,
        .name = filename,
    };

    asset.instance = gs_audio_instance_create(
        &(gs_audio_instance_decl_t){
            .src = asset.source,
            .persistent = persistent,
            .loop = loop,
            .volume = g_audio_manager->master_vol * g_audio_manager->mixer_vol[type],
        });

    gs_dyn_array_push(g_audio_manager->assets, asset);
}

mg_audio_asset_t *_mg_audio_manager_find(char *name)
{
    mg_audio_asset_t *asset = NULL;

    for (size_t i = 0; i < gs_dyn_array_size(g_audio_manager->assets); i++)
    {
        if (strcmp(g_audio_manager->assets[i].name, name) == 0)
        {
            asset = &g_audio_manager->assets[i];
            break;
        }
    }

    return asset;
}