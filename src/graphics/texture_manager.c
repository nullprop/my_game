/*================================================================
	* graphics/texture_manager.c
	*
	* Copyright (c) 2021 Lauri Räsänen
	* ================================

	Loading and storing texture pointers by filename.
=================================================================*/

#include "texture_manager.h"
#include "../game/config.h"
#include "../game/console.h"
#include "../util/string.h"

mg_texture_manager_t *g_texture_manager;

void mg_texture_manager_init()
{
	g_texture_manager	    = gs_malloc_init(mg_texture_manager_t);
	g_texture_manager->textures = gs_dyn_array_new(mg_texture_t);

	g_texture_manager->tex_filter = mg_cvar("r_filter")->value.i;
	g_texture_manager->mip_filter = mg_cvar("r_filter_mip")->value.i;
	g_texture_manager->num_mips   = mg_cvar("r_mips")->value.i;
}

void mg_texture_manager_free()
{
	for (size_t i = 0; i < gs_dyn_array_size(g_texture_manager->textures); i++)
	{
		gs_graphics_texture_destroy(g_texture_manager->textures[i].asset->hndl);
		g_texture_manager->textures[i].asset->hndl = gs_handle_invalid(gs_graphics_texture_t);
		gs_free(g_texture_manager->textures[i].asset);
		gs_free(g_texture_manager->textures[i].filename);
	}

	gs_dyn_array_free(g_texture_manager->textures);

	gs_free(g_texture_manager);
	g_texture_manager = NULL;
}

void mg_texture_manager_set_filter(gs_graphics_texture_filtering_type tex, gs_graphics_texture_filtering_type mip, int num_mips)
{
	g_texture_manager->tex_filter = tex;
	g_texture_manager->mip_filter = mip;
	g_texture_manager->num_mips   = num_mips;
	for (size_t i = 0; i < gs_dyn_array_size(g_texture_manager->textures); i++)
	{
		if (
			g_texture_manager->textures[i].asset->desc.min_filter == tex &&
			g_texture_manager->textures[i].asset->desc.mag_filter == tex &&
			g_texture_manager->textures[i].asset->desc.mip_filter == mip &&
			g_texture_manager->textures[i].asset->desc.num_mips == num_mips)
		{
			continue;
		}

		gs_graphics_texture_destroy(g_texture_manager->textures[i].asset->hndl);
		gs_assert(
			_mg_texture_manager_load(
				g_texture_manager->textures[i].filename,
				g_texture_manager->textures[i].asset));
	}
}

// Get texture pointer, load from file if required.
// Returns NULL on failure.
gs_asset_texture_t *mg_texture_manager_get(char *path)
{
	// Strip any extensions from path
	char *filename = mg_path_remove_ext(path);

	gs_asset_texture_t *asset = _mg_texture_manager_find(filename);
	if (asset != NULL)
	{
		gs_free(filename);
		return asset;
	}

	asset		 = gs_malloc_init(gs_asset_texture_t);
	bool32_t success = _mg_texture_manager_load(filename, asset);
	if (!success)
	{
		gs_free(asset);
		gs_free(filename);
		return NULL;
	}

	mg_texture_t tex = (mg_texture_t){
		.asset	  = asset,
		.filename = filename,
	};
	gs_dyn_array_push(g_texture_manager->textures, tex);

	return asset;
}

// Load gs_asset_texture from a file.
bool32_t _mg_texture_manager_load(char *name, gs_asset_texture_t *asset)
{
	// Supported extensions
	char extensions[2][5] = {
		".jpg",
		".tga",
	};

	bool32_t success = false;

	// Name with extension
	size_t malloc_sz = strlen(name) + 5;
	char *filename	 = gs_malloc(malloc_sz);
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
					.format	    = GS_GRAPHICS_TEXTURE_FORMAT_RGBA8,
					.min_filter = g_texture_manager->tex_filter,
					.mag_filter = g_texture_manager->tex_filter,
					.mip_filter = g_texture_manager->mip_filter,
					.num_mips   = g_texture_manager->num_mips,
				},
				false,
				false);
		}
		else if (i == 1)
		{
			mg_println("WARN: _mg_texture_manager_load could not load texture: %s, file not found", name);
		}

		if (success)
		{
			mg_println("_mg_texture_manager_load: Loaded texture: %s", name);
			break;
		}
		else
		{
			asset->hndl = gs_handle_invalid(gs_graphics_texture_t);
		}
	}

	gs_free(filename);

	return success;
}

gs_asset_texture_t *_mg_texture_manager_find(char *filename)
{
	for (size_t i = 0; i < gs_dyn_array_size(g_texture_manager->textures); i++)
	{
		if (strcmp(g_texture_manager->textures[i].filename, filename) == 0)
		{
			return g_texture_manager->textures[i].asset;
		}
	}

	return NULL;
}
