/*================================================================
	* bsp/bsp_loader.c
	*
	* Copyright (c) 2021 Lauri Räsänen
	* Copyright (c) 2018 Krzysztof Kondrak
	*
	* See README.md for license.
	* ================================

	Handles BSP map loading from file.
	NOTE: only supports version 46.
=================================================================*/

#include "bsp_loader.h"
#include "../game/console.h"

// shorthand util for failing during BSP load
b32 _load_bsp_fail(gs_byte_buffer_t *buffer, char *msg)
{
	mg_println("load_bsp() failed: %s", msg);
	gs_byte_buffer_free(buffer);
	return false;
}

// load BSP from file
b32 load_bsp(char *filename, bsp_map_t *map)
{
	mg_println("load_bsp() loading: '%s'", filename);

	if (!gs_util_file_exists(filename))
	{
		mg_println("load_bsp() failed: file not found '%s'", filename);
		return false;
	}

	gs_byte_buffer_t buffer = gs_byte_buffer_new();
	gs_byte_buffer_read_from_file(&buffer, filename);

	// read header
	if (!_load_bsp_header(&buffer, &map->header))
		return _load_bsp_fail(&buffer, "failed to read header");

	// validate header
	if (memcmp(map->header.magic, "IBSP", 4) != 0 || map->header.version != 46)
		return _load_bsp_fail(&buffer, "invalid header");

	// read entity lump
	if (!_load_entity_lump(&buffer, map))
		return _load_bsp_fail(&buffer, "failed to read entity lumps");

	// generic lumps
	uint32_t *counts[] = {
		&map->textures.count,
		&map->planes.count,
		&map->nodes.count,
		&map->leaves.count,
		&map->leaf_faces.count,
		&map->leaf_brushes.count,
		&map->models.count,
		&map->brushes.count,
		&map->brush_sides.count,
		&map->vertices.count,
		&map->indices.count,
		&map->effects.count,
		&map->faces.count,
		&map->lightmaps.count,
		&map->lightvols.count,
	};
	void **datas[] = {
		&map->textures.data,
		&map->planes.data,
		&map->nodes.data,
		&map->leaves.data,
		&map->leaf_faces.data,
		&map->leaf_brushes.data,
		&map->models.data,
		&map->brushes.data,
		&map->brush_sides.data,
		&map->vertices.data,
		&map->indices.data,
		&map->effects.data,
		&map->faces.data,
		&map->lightmaps.data,
		&map->lightvols.data,
	};
	uint32_t sizes[] = {
		sizeof(bsp_texture_lump_t),
		sizeof(bsp_plane_lump_t),
		sizeof(bsp_node_lump_t),
		sizeof(bsp_leaf_lump_t),
		sizeof(bsp_leaf_face_lump_t),
		sizeof(bsp_leaf_brush_lump_t),
		sizeof(bsp_model_lump_t),
		sizeof(bsp_brush_lump_t),
		sizeof(bsp_brush_side_lump_t),
		sizeof(bsp_vert_lump_t),
		sizeof(bsp_index_lump_t),
		sizeof(bsp_effect_lump_t),
		sizeof(bsp_face_lump_t),
		sizeof(bsp_lightmap_lump_t),
		sizeof(bsp_lightvol_lump_t),
	};

	// read generics
	uint32_t start = BSP_LUMP_TYPE_TEXTURES;
	uint32_t end   = BSP_LUMP_TYPE_LIGHTVOLS;
	for (size_t i = start; i <= end; i++)
	{
		if (!_load_lump(&buffer, map, counts[i - start], datas[i - start], i, sizes[i - start]))
		{
			gs_byte_buffer_free(&buffer);
			return false;
		}
	}

	// read visdata lump
	if (!_load_visdata_lump(&buffer, map))
		return _load_bsp_fail(&buffer, "failed to read visdata lump");

	map->valid = map->faces.count > 0;
	gs_byte_buffer_free(&buffer);

	// Get map name from filepath
	char *name = mg_get_filename_from_path(filename);
	size_t sz  = strlen(name) + 1;
	map->name  = gs_malloc(sz);
	memcpy(map->name, name, sz);

	mg_println("load_bsp() done loading '%s'", map->name);

	return true;
}

b32 _load_bsp_header(gs_byte_buffer_t *buffer, bsp_header_t *header)
{
	gs_byte_buffer_read(buffer, bsp_header_t, header);

	return true;
}

b32 _load_lump(gs_byte_buffer_t *buffer, bsp_map_t *map, uint32_t *count, void **data, bsp_lump_types type, uint32_t lump_size)
{
	uint32_t size = map->header.dir_entries[type].length;

	*count = size / lump_size;
	*data  = gs_malloc(size);

	buffer->position = map->header.dir_entries[type].offset;
	gs_byte_buffer_read_bulk(buffer, data, size);

	return true;
}

b32 _load_entity_lump(gs_byte_buffer_t *buffer, bsp_map_t *map)
{
	int32_t size = map->header.dir_entries[BSP_LUMP_TYPE_ENTITIES].length;

	// not sure if the lump is null terminated,
	// malloc extra byte for \0 at the end.
	map->entity_lump.ents = gs_malloc(size + 1);

	buffer->position = map->header.dir_entries[BSP_LUMP_TYPE_ENTITIES].offset;
	gs_byte_buffer_read_bulk(buffer, &map->entity_lump.ents, size);
	map->entity_lump.ents[size] = '\0';

	return true;
}

b32 _load_visdata_lump(gs_byte_buffer_t *buffer, bsp_map_t *map)
{
	buffer->position = map->header.dir_entries[BSP_LUMP_TYPE_VISDATA].offset;
	gs_byte_buffer_read(buffer, int32_t, &map->visdata.num_vecs);
	gs_byte_buffer_read(buffer, int32_t, &map->visdata.size_vecs);

	int32_t size	  = map->visdata.num_vecs * map->visdata.size_vecs;
	map->visdata.vecs = gs_malloc(size);
	gs_byte_buffer_read_bulk(buffer, &map->visdata.vecs, size);

	return true;
}