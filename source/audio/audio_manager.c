/*================================================================
    * audio/audio_manager.c
    *
    * Copyright (c) 2021 Lauri Räsänen
    * ================================

    ...
=================================================================*/

#include <gs/gs.h>

#include "../util/config.h"
#include "../util/math.h"
#include "../util/string.h"
#include "audio_manager.h"

mg_audio_manager_t *g_audio_manager;

void mg_audio_manager_init()
{
    // Allocate
    g_audio_manager = gs_malloc_init(mg_audio_manager_t);
    g_audio_manager->assets = gs_dyn_array_new(mg_audio_asset_t);

    // Config
    g_audio_manager->master_vol = g_config->sound.master;
    g_audio_manager->mixer_vol[MG_AUDIO_TYPE_EFFECT] = g_config->sound.effect;
    g_audio_manager->mixer_vol[MG_AUDIO_TYPE_MUSIC] = g_config->sound.music;
    g_audio_manager->mixer_vol[MG_AUDIO_TYPE_AMBIENT] = g_config->sound.ambient;

    // Player sounds
    _mg_audio_manager_load("sound/player/jump1.wav", MG_AUDIO_TYPE_EFFECT, false, false, 1.0f);
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

void mg_audio_manager_play(char *name, float pitch_var)
{
    mg_audio_asset_t *asset = _mg_audio_manager_find(name);
    if (asset == NULL)
    {
        gs_println("WARN: mg_audio_manager_play invalid audio %s", name);
        return;
    }

    float pitch = 0.0f;
    if (pitch_var != 0.0f)
    {
        pitch = (float)rand_range(-512, 512) * pitch_var / 512.0f;
    }

    gs_audio_instance_decl_t data = gs_audio_get_instance_data(asset->instance);
    data.pitch = pitch;
    gs_audio_set_instance_data(asset->instance, data);

    // FIXME: playing instance doesn't work here
    if (asset->loop)
    {
        gs_audio_play(asset->instance);
    }
    else
    {
        //gs_audio_play(asset->instance);
        gs_audio_play_source(asset->source, g_audio_manager->master_vol * g_audio_manager->mixer_vol[asset->type] * asset->volume, pitch);
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

void _mg_audio_manager_load(char *filename, mg_audio_type type, bool loop, bool persistent, float volume)
{
    mg_audio_asset_t asset = (mg_audio_asset_t){
        .source = gs_audio_load_from_file(filename),
        .type = type,
        .loop = loop,
        .persistent = persistent,
        .name = filename,
        .volume = volume,
    };

    if (!gs_handle_is_valid(asset.source))
    {
        return;
    }

    asset.instance = gs_audio_instance_create(
        &(gs_audio_instance_decl_t){
            .src = asset.source,
            .persistent = persistent,
            .loop = loop,
            .volume = volume * g_audio_manager->master_vol * g_audio_manager->mixer_vol[type],
        });
    
    if (!gs_handle_is_valid(asset.instance))
    {
        return;
    }

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