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
	g_config	= gs_malloc(sizeof(mg_config_t));
	g_config->cvars = gs_dyn_array_new(mg_cvar_t);

	mg_cvar_new("vid_fullscreen", MG_CONFIG_TYPE_INT, 0);
	mg_cvar_new("vid_width", MG_CONFIG_TYPE_INT, 800);
	mg_cvar_new("vid_height", MG_CONFIG_TYPE_INT, 600);
	mg_cvar_new("vid_max_fps", MG_CONFIG_TYPE_INT, 240);
	mg_cvar_new("vid_vsync", MG_CONFIG_TYPE_INT, 0);

	mg_cvar_new("snd_master", MG_CONFIG_TYPE_FLOAT, 0.1f);
	mg_cvar_new("snd_effect", MG_CONFIG_TYPE_FLOAT, 0.6f);
	mg_cvar_new("snd_music", MG_CONFIG_TYPE_FLOAT, 1.0f);
	mg_cvar_new("snd_ambient", MG_CONFIG_TYPE_FLOAT, 1.0f);

	mg_cvar_new("r_fov", MG_CONFIG_TYPE_INT, 115);
	mg_cvar_new("r_barrel_enabled", MG_CONFIG_TYPE_INT, 1);
	mg_cvar_new("r_barrel_strength", MG_CONFIG_TYPE_FLOAT, 0.5f);
	mg_cvar_new("r_barrel_cyl_ratio", MG_CONFIG_TYPE_FLOAT, 1.0f);
	mg_cvar_new("r_filter", MG_CONFIG_TYPE_INT, 0);
	mg_cvar_new("r_filter_mip", MG_CONFIG_TYPE_INT, 1);
	mg_cvar_new("r_mips", MG_CONFIG_TYPE_INT, 0);

	mg_cvar_new("cl_sensitivity", MG_CONFIG_TYPE_FLOAT, 2.0f);

	mg_cvar_new_str("stringtest", MG_CONFIG_TYPE_STRING, "Sandvich make me strong!");

	// Load config if exists
	if (gs_platform_file_exists("assets/cfg/config.txt"))
	{
		_mg_config_load("assets/cfg/config.txt");
	}
	else
	{
		mg_println("WARN: missing assets/cfg/config.txt, saving default");
		_mg_config_save("assets/cfg/config.txt");
	}

	mg_cmd_new("cvars", "Shows available cvars", &mg_config_print, NULL, 0);
}

void mg_config_free()
{
	_mg_config_save("assets/cfg/config.txt");

	for (size_t i = 0; i < gs_dyn_array_size(g_config->cvars); i++)
	{
		if (g_config->cvars->type == MG_CONFIG_TYPE_STRING)
		{
			gs_free(g_config->cvars[i].value.s);
			g_config->cvars[i].value.s = NULL;
		}
	}
	gs_dyn_array_free(g_config->cvars);

	gs_free(g_config);
	g_config = NULL;
}

mg_cvar_t *mg_config_get(char *name)
{
	for (size_t i = 0; i < gs_dyn_array_size(g_config->cvars); i++)
	{
		if (strcmp(g_config->cvars[i].name, name) == 0)
		{
			return &g_config->cvars[i];
		}
	}
	return NULL;
}

void mg_config_print()
{
	for (size_t i = 0; i < gs_dyn_array_size(g_config->cvars); i++)
	{
		mg_cvar_print(&g_config->cvars[i]);
	}
}

// Load config from file
void _mg_config_load(char *filepath)
{
	if (!gs_platform_file_exists(filepath))
	{
		mg_println("WARN: failed to read config file %s", filepath);
		return;
	}

	mg_println("Loading config from '%s'", filepath);

	char *file_data = gs_platform_read_file_contents(filepath, "r", NULL);

	char *line;
	char *line_ptr;
	char *token;
	u8 num_line = 0;
	line = strtok_r(file_data, "\r\n", &line_ptr);
	while (line != NULL)
	{
		num_line++;
		//gs_println("line %d: %s", num_line, line);

		// Empty line
		if (line[0] == '\n')
		{
			line = strtok_r(NULL, "\r\n", &line_ptr);
			continue;
		}

		// Comment
		if (line[0] == '/' && line[1] == '/')
		{
			line = strtok_r(NULL, "\r\n", &line_ptr);
			continue;
		}

		token = strtok(line, " ");
		if (!token)
		{
			line = strtok_r(NULL, "\r\n", &line_ptr);
		}

		mg_cvar_t *cvar = mg_cvar(token);
		if (cvar == NULL)
		{
			mg_println("WARN: _mg_config_load unknown cvar name %s", token);
			line = strtok_r(NULL, "\r\n", &line_ptr);
			continue;
		}

		if (cvar->type == MG_CONFIG_TYPE_STRING)
		{
			token = strtok(NULL, "\n");
		}
		else
		{
			token = strtok(NULL, " ");
		}

		if (!token)
		{
			line = strtok_r(NULL, "\r\n", &line_ptr);
			continue;
		}

		switch (cvar->type)
		{
		case MG_CONFIG_TYPE_INT:
			cvar->value.i = strtol(token, (char **)NULL, 10);
			break;

		case MG_CONFIG_TYPE_FLOAT:
			cvar->value.f = strtof(token, (char **)NULL);
			break;

		case MG_CONFIG_TYPE_STRING:
			memset(cvar->value.s, 0, MG_CVAR_STR_LEN);
			memcpy(cvar->value.s, token, gs_min(MG_CVAR_STR_LEN - 1, gs_string_length(token)));
			break;

		default:
			mg_println("WARN: _mg_config_load unknown cvar type %d", cvar->type);
			break;
		}

		line = strtok_r(NULL, "\r\n", &line_ptr);
	}

	gs_free(file_data);
	mg_println("Config loaded");
}

// Save current config to filepath.
// User formatting and replacing values is a hassle,
// let's just dump all cvars and overwrite.
void _mg_config_save(char *filepath)
{
	mg_println("Saving config to '%s'", filepath);

	FILE *file = gs_platform_open_file(filepath, "w");
	if (file == NULL)
	{
		mg_println("WARN: _mg_config_save couldn't save config to '%s'", filepath);
		return;
	}

	char line[128];

	for (size_t i = 0; i < gs_dyn_array_size(g_config->cvars); i++)
	{
		mg_cvar_t cvar = g_config->cvars[i];
		gs_fprintf(file, "%s ", g_config->cvars[i].name);
		switch (g_config->cvars[i].type)
		{
		case MG_CONFIG_TYPE_INT:
			gs_fprintf(file, "%d\n", g_config->cvars[i].value.i);
			break;

		case MG_CONFIG_TYPE_FLOAT:
			gs_fprintf(file, "%f\n", g_config->cvars[i].value.f);
			break;

		case MG_CONFIG_TYPE_STRING:
			gs_fprintf(file, "%s\n", g_config->cvars[i].value.s);
			break;

		default:
			mg_println("WARN: _mg_config_save unknown cvar type %d", g_config->cvars[i].type);
			gs_fprintf(file, "%d\n", g_config->cvars[i].value.i);
			break;
		}
	}

	fflush(file);
	fclose(file);
	mg_println("Config saved");
}
