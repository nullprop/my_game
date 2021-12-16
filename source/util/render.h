/*================================================================
    * util/render.h
    *
    * Copyright (c) 2021 Lauri Räsänen
    * ================================

    Rendering utilities.
=================================================================*/

#ifndef MG_RENDER_H
#define MG_RENDER_H

#include <gs/gs.h>

// Generate black and pink grid for a missing texture
static inline gs_color_t *mg_get_missing_texture_pixels(uint32_t size)
{
    gs_color_t pink = gs_color(255, 0, 220, 255);
    gs_color_t black = gs_color(0, 0, 0, 255);
    gs_color_t *pixels = (gs_color_t *)gs_malloc(size * size * sizeof(gs_color_t));
    for (uint32_t row = 0; row < size; row++)
    {
        for (uint32_t col = 0; col < size; col++)
        {
            uint32_t idx = row * size + col;
            if ((row % 2) == (col % 2))
            {
                pixels[idx] = pink;
            }
            else
            {
                pixels[idx] = black;
            }
        }
    }

    return pixels;
}

// Load gs_asset_texture from a file.
static inline bool mg_load_texture_asset(char *path, gs_asset_texture_t *asset)
{
    // Strip any extensions from path
    size_t malloc_sz = strlen(path) + 1;
    char *name = gs_malloc(malloc_sz);
    bool32_t end = false;
    for (size_t i = 0; i < malloc_sz; i++)
    {
        if (path[i] == '.')
        {
            end = true;
        }

        name[i] = end ? '\0' : path[i];
    }

    // Supported extensions
    char extensions[2][5] = {
        ".jpg",
        ".tga",
    };

    bool32_t success = false;

    // Name with extension
    malloc_sz = strlen(name) + 5;
    char *filename = gs_malloc(malloc_sz);
    memset(filename, 0, malloc_sz);
    strcat(filename, name);
    strcat(filename, extensions[0]);

    for (size_t i = 0; i < 2; i++)
    {
        if (i > 0)
        {
            strcpy(filename + strlen(filename) - 4, extensions[i]);
        }

        if (gs_util_file_exists(filename))
        {
            success = gs_asset_texture_load_from_file(
                filename,
                asset,
                &(gs_graphics_texture_desc_t){
                    .format = GS_GRAPHICS_TEXTURE_FORMAT_RGBA8,
                    .min_filter = GS_GRAPHICS_TEXTURE_FILTER_NEAREST,
                    .mag_filter = GS_GRAPHICS_TEXTURE_FILTER_NEAREST,
                },
                false,
                false);
        }
        else if (i == 1)
        {
            gs_println("Warning: could not load texture: %s, file not found", name);
        }

        if (success)
        {
            gs_println("Loaded texture: %s", name);
            break;
        }
        else
        {
            asset->hndl = gs_handle_invalid(gs_graphics_texture_t);
        }
    }

    gs_free(name);
    gs_free(filename);

    return success;
}

#endif // MG_RENDER_H