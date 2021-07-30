/*================================================================
    * util/config.c
    *
    * Copyright (c) 2021 Lauri Räsänen
    * ================================

    Handle config files.
=================================================================*/

#include <gs/gs.h>

#include "config.h"

static mg_config_t *mg_config = NULL;

void _mg_config_load(char *filepath);
void _mg_config_save(char *filepath);
void _mg_config_set_default();

void mg_config_init()
{
    mg_config = gs_malloc(sizeof(mg_config_t));

    // Always set defaults before loading
    // in case we add new config keys and use old file.
    _mg_config_set_default();

    // Load config if exists
    if (gs_util_file_exists("cfg/config.txt"))
    {
        _mg_config_load("cfg/config.txt");
        return;
    }

    gs_println("WARN: missing cfg/config.txt, saving default");
    _mg_config_save("cfg/config.txt");
}

void mg_config_free()
{
    gs_free(mg_config);
    mg_config = NULL;
}

// Load config from file
void _mg_config_load(char *filepath)
{
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
    u8 num_parts;
    uint16_t num_line;
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
        token = strtok(&line, " ");
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

            token = strtok(0, " ");
        }

        // Check we got both
        if (num_parts < 2)
        {
            gs_println("WARN: config line %zu has too few arguments", num_line);
            continue;
        }

// Handle known config keys
#define NUM_KEYS 5
        char *known_keys[NUM_KEYS] = {
            "vid_fullscreen",
            "vid_width",
            "vid_height",
            "vid_max_fps",
            "vid_vsync",
        };
        int32_t *containers[NUM_KEYS] = {
            &mg_config->video.fullscreen,
            &mg_config->video.width,
            &mg_config->video.height,
            &mg_config->video.max_fps,
            &mg_config->video.vsync,
        };
        bool32_t found_key = false;

        for (size_t i = 0; i < NUM_KEYS; i++)
        {
            if (gs_string_compare_equal(&key, known_keys[i]))
            {
                *containers[i] = strtol(value, (char **)NULL, 10);
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
    fprintf(file, "vid_fullscreen %d\n", mg_config->video.fullscreen);
    fprintf(file, "vid_width %d\n", mg_config->video.width);
    fprintf(file, "vid_height %d\n", mg_config->video.height);
    fprintf(file, "vid_max_fps %d\n", mg_config->video.max_fps);
    fprintf(file, "vid_vsync %d\n", mg_config->video.vsync);

    fprintf(file, "\n");
    fprintf(file, "// Audio\n");

    fprintf(file, "\n");
    fprintf(file, "// Graphics\n");

    fflush(file);
    fclose(file);
    gs_println("Config saved");
}

void _mg_config_set_default()
{
    // Video
    mg_config->video.fullscreen = false;
    mg_config->video.width = 1280;
    mg_config->video.height = 720;
    mg_config->video.max_fps = 240;
    mg_config->video.vsync = false;

    // Audio

    // Graphics
}