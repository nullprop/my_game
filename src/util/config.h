/*================================================================
	* util/config.h
	*
	* Copyright (c) 2021 Lauri Räsänen
	* ================================

	Config types.
=================================================================*/

#ifndef MG_CONFIG_H
#define MG_CONFIG_H

#include <gs/gs.h>

typedef enum mg_config_type
{
	MG_CONFIG_TYPE_INT,
	MG_CONFIG_TYPE_FLOAT,
	MG_CONFIG_TYPE_KEY, // TODO
	MG_CONFIG_TYPE_COUNT,
} mg_config_type;

typedef struct mg_config_video_t
{
	bool32_t fullscreen;
	uint32_t width;
	uint32_t height;
	uint32_t max_fps;
	bool32_t vsync;
} mg_config_video_t;

typedef struct mg_config_graphics_t
{
	uint32_t fov;
	bool32_t barrel_enabled;
	float32_t barrel_strength;
	float32_t barrel_cyl_ratio;
} mg_config_graphics_t;

typedef struct mg_config_sound_t
{
	float32_t master;
	float32_t effect;
	float32_t music;
	float32_t ambient;
} mg_config_sound_t;

typedef struct mg_config_controls_t
{
	float32_t sensitivity;
} mg_config_controls_t;

typedef struct mg_config_t
{
	mg_config_video_t video;
	mg_config_graphics_t graphics;
	mg_config_sound_t sound;
	mg_config_controls_t controls;
} mg_config_t;

void mg_config_init();
void mg_config_free();
void _mg_config_load(char *filepath);
void _mg_config_save(char *filepath);
void _mg_config_set_default();

extern mg_config_t *g_config;

#endif // MG_CONFIG_H