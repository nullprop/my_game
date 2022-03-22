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
	g_audio_manager		= gs_malloc_init(mg_audio_manager_t);
	g_audio_manager->assets = gs_dyn_array_new(mg_audio_asset_t);

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

	mg_cvar_t *snd_master = mg_cvar("snd_master");
	mg_cvar_t *snd_mixer  = NULL;
	switch (asset->type)
	{
	case MG_AUDIO_TYPE_EFFECT:
		snd_mixer = mg_cvar("snd_effect");
		break;

	case MG_AUDIO_TYPE_MUSIC:
		snd_mixer = mg_cvar("snd_music");
		break;

	case MG_AUDIO_TYPE_AMBIENT:
		snd_mixer = mg_cvar("snd_ambient");
		break;

	default:
		gs_println("WARN: mg_audio_manager_play invalid type %d", asset->type);
		snd_mixer = snd_master;
		break;
	}

	float pitch = 0.0f;
	if (pitch_var != 0.0f)
	{
		pitch = (float)rand_range(-512, 512) * pitch_var / 512.0f;
	}

	gs_audio_instance_decl_t data = gs_audio_get_instance_data(asset->instance);
#ifdef PITCH
	data.pitch = pitch;
#endif
	gs_audio_set_instance_data(asset->instance, data);

	// FIXME: playing instance doesn't work here
	if (asset->loop)
	{
		gs_audio_play(asset->instance);
	}
	else
	{
		// gs_audio_play(asset->instance);
#ifdef PITCH
		gs_audio_play_source(asset->source, snd_master->value.f * snd_mixer->value.f * asset->volume, pitch);
#else
		gs_audio_play_source(asset->source, snd_master->value.f * snd_mixer->value.f * asset->volume);
#endif
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
		.source	    = gs_audio_load_from_file(filename),
		.type	    = type,
		.loop	    = loop,
		.persistent = persistent,
		.name	    = filename,
		.volume	    = volume,
	};

	if (!gs_handle_is_valid(asset.source))
	{
		return;
	}

	asset.instance = gs_audio_instance_create(
		&(gs_audio_instance_decl_t){
			.src	    = asset.source,
			.persistent = persistent,
			.loop	    = loop,
			.volume	    = volume,
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