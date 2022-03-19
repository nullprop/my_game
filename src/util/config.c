/*================================================================
	* util/config.c
	*
	* Copyright (c) 2021 Lauri Räsänen
	* ================================

	Handle config files.
=================================================================*/

#include <gs/gs.h>

#include "config.h"

mg_config_t *g_config;

void mg_config_init()
{
	g_config = gs_malloc(sizeof(mg_config_t));

	// Always set defaults before loading
	// in case we add new config keys and use old file.
	_mg_config_set_default();

	// Load config if exists
	if (gs_util_file_exists("cfg/config.txt"))
	{
		_mg_config_load("cfg/config.txt");
	}
	else
	{
		gs_println("WARN: missing cfg/config.txt, saving default");
		_mg_config_save("cfg/config.txt");
	}
}

void mg_config_free()
{
	gs_free(g_config);
	g_config = NULL;
}

// Load config from file
void _mg_config_load(char *filepath)
{
	// Known config keys
	// TODO: could make a macro to simplify these
#define NUM_KEYS 11
	char *known_keys[NUM_KEYS] = {
		// Video
		"vid_fullscreen",
		"vid_width",
		"vid_height",
		"vid_max_fps",
		"vid_vsync",
		// Sound
		"snd_master",
		"snd_effect",
		"snd_music",
		"snd_ambient",
		// Graphics
		"r_fov",
		// Controls
		"cl_sensitivity",
	};
	void *containers[NUM_KEYS] = {
		// Video
		&g_config->video.fullscreen,
		&g_config->video.width,
		&g_config->video.height,
		&g_config->video.max_fps,
		&g_config->video.vsync,
		// Audio
		&g_config->sound.master,
		&g_config->sound.effect,
		&g_config->sound.music,
		&g_config->sound.ambient,
		// Graphics
		&g_config->graphics.fov,
		// Controls
		&g_config->controls.sensitivity,
	};
	mg_config_type types[NUM_KEYS] = {
		// Video
		MG_CONFIG_TYPE_INT,
		MG_CONFIG_TYPE_INT,
		MG_CONFIG_TYPE_INT,
		MG_CONFIG_TYPE_INT,
		MG_CONFIG_TYPE_INT,
		// Audio
		MG_CONFIG_TYPE_FLOAT,
		MG_CONFIG_TYPE_FLOAT,
		MG_CONFIG_TYPE_FLOAT,
		MG_CONFIG_TYPE_FLOAT,
		// Graphics
		MG_CONFIG_TYPE_INT,
		// Controls
		MG_CONFIG_TYPE_FLOAT,
	};

	gs_println("Loading config from '%s'", filepath);

	FILE *file = fopen(filepath, "r");
	if (file == NULL)
	{
		gs_println("WARN: failed to read config file %s", filepath);
		return;
	}

	char line[128];
	char key[64];
	char value[64];
	char *token;
	u8 num_parts = 0;
	u8 num_line  = 0;
	while (fgets(line, sizeof(line), file))
	{
		num_line++;

		// Empty line
		if (line[0] == '\n')
		{
			continue;
		}

		// Comment
		if (line[0] == '/' && line[1] == '/')
		{
			continue;
		}

		// Clear previous
		memset(&key, 0, 64);
		memset(&value, 0, 64);

		// Assign key and value
		num_parts = 0;
		token	  = strtok(&line, " ");
		while (token)
		{
			switch (num_parts)
			{
			case 0:
				strcat(&key, token);
				break;

			case 1:
				strcat(&value, token);
				break;

			// Should never get here
			default:
				gs_println("WARN: config line %zu has too many arguments", num_line);
				break;
			}

			num_parts++;

			if (num_parts == 2)
			{
				// Ignore rest
				break;
			}

			token = strtok(NULL, " ");
		}

		// Check we got both
		if (num_parts < 2)
		{
			gs_println("WARN: config line %zu has too few arguments", num_line);
			continue;
		}

		// Find the key
		bool32_t found_key = false;
		for (size_t i = 0; i < NUM_KEYS; i++)
		{
			if (gs_string_compare_equal(&key, known_keys[i]))
			{
				if (types[i] == MG_CONFIG_TYPE_INT)
				{
					*(int32_t *)containers[i] = strtol(value, (char **)NULL, 10);
				}
				else if (types[i] == MG_CONFIG_TYPE_FLOAT)
				{
					*(float32_t *)containers[i] = strtof(value, (char **)NULL);
				}

				found_key = true;
				break;
			}
		}

		if (!found_key)
		{
			gs_println("WARN: unknown config key %s", key);
		}
	}

	fclose(file);
	gs_println("Config loaded");
}

// Save current config to filepath.
// User formatting and replacing values is a hassle,
// let's just overwrite the file with our format and comments.
void _mg_config_save(char *filepath)
{
	gs_println("Saving config to '%s'", filepath);

	FILE *file = fopen(filepath, "w");
	if (file == NULL)
	{
		gs_println("WARN: couldn't save config to '%s'", filepath);
		return;
	}

	char line[128];

	fprintf(file, "// Video\n");
	fprintf(file, "vid_fullscreen %zu\n", g_config->video.fullscreen);
	fprintf(file, "vid_width %zu\n", g_config->video.width);
	fprintf(file, "vid_height %zu\n", g_config->video.height);
	fprintf(file, "vid_max_fps %zu\n", g_config->video.max_fps);
	fprintf(file, "vid_vsync %zu\n", g_config->video.vsync);

	fprintf(file, "\n");
	fprintf(file, "// Audio\n");
	fprintf(file, "snd_master %f\n", g_config->sound.master);
	fprintf(file, "snd_effect %f\n", g_config->sound.effect);
	fprintf(file, "snd_music %f\n", g_config->sound.music);
	fprintf(file, "snd_ambient %f\n", g_config->sound.ambient);

	fprintf(file, "\n");
	fprintf(file, "// Graphics\n");
	fprintf(file, "r_fov %zu\n", g_config->graphics.fov);

	fprintf(file, "\n");
	fprintf(file, "// Controls\n");
	fprintf(file, "cl_sensitivity %f\n", g_config->controls.sensitivity);

	fflush(file);
	fclose(file);
	gs_println("Config saved");
}

void _mg_config_set_default()
{
	// Video
	g_config->video.fullscreen = false;
	g_config->video.width	   = 800;
	g_config->video.height	   = 600;
	g_config->video.max_fps	   = 240;
	g_config->video.vsync	   = false;

	// Audio
	g_config->sound.master	= 0.1f;
	g_config->sound.effect	= 0.6f;
	g_config->sound.music	= 1.0f;
	g_config->sound.ambient = 0.6f;

	// Graphics
	g_config->graphics.fov = 110;

	// Controls
	g_config->controls.sensitivity = 2.0f;
}