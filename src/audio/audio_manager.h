/*================================================================
	* audio/audio_manager.h
	*
	* Copyright (c) 2021 Lauri Räsänen
	* ================================

	...
=================================================================*/

#ifndef MG_AUDIO_MANAGER_H
#define MG_AUDIO_MANAGER_H

#include <gs/gs.h>

typedef enum mg_audio_type
{
	MG_AUDIO_TYPE_EFFECT,
	MG_AUDIO_TYPE_MUSIC,
	MG_AUDIO_TYPE_AMBIENT,
	MG_AUDIO_TYPE_COUNT,
} mg_audio_type;

typedef struct mg_audio_asset_t
{
	gs_handle(gs_audio_source_t) source;
	gs_handle(gs_audio_instance_t) instance;
	mg_audio_type type;
	bool loop;
	bool persistent;
	char *name;
	float volume;
} mg_audio_asset_t;

typedef struct mg_audio_manager_t
{
	gs_dyn_array(mg_audio_asset_t) assets;
} mg_audio_manager_t;

void mg_audio_manager_init();
void mg_audio_manager_free();
void mg_audio_manager_play(char *name, float pitch_var);
void mg_audio_manager_stop(char *name);
void mg_audio_manager_pause(char *name);
void mg_audio_manager_restart(char *name);
bool mg_audio_manager_is_playing(char *name);
void _mg_audio_manager_load(char *filename, mg_audio_type type, bool loop, bool persistent, float volume);
mg_audio_asset_t *_mg_audio_manager_find(char *name);

extern mg_audio_manager_t *g_audio_manager;

#endif // MG_AUDIO_MANAGER_H