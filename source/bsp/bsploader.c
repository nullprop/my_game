/*================================================================
    * bsp/bsploader.c
    *
    * Copyright (c) 2021 Lauri Räsänen
    * Copyright (c) 2018 Krzysztof Kondrak
    *
    * See README.md for license.
    * ================================

    Handles BSP map loading from file.
    NOTE: only supports version 46.
=================================================================*/

#include <gs/gs.h>
#include "bsptypes.h"

// shorthand util for failing during BSP load
b32 _load_bsp_fail(gs_byte_buffer_t *buffer, char *msg)
{
    gs_println("load_bsp() failed: %s", msg);
    gs_byte_buffer_free(buffer);
    return false;
}

// load BSP from file
b32 load_bsp(char *filename, bsp_map_t *map)
{
    gs_println("load_bsp() loading: '%s'", filename);

    if (!gs_util_file_exists(filename))
    {
        gs_println("load_bsp() failed: file not found '%s'", filename);
        return false;
    }

    gs_byte_buffer_t buffer = gs_byte_buffer_new();
    gs_byte_buffer_read_from_file(&buffer, filename);

    map = gs_malloc_init(bsp_map_t);

    // read header
    if (!_load_bsp_header(&buffer, &map->header))
        return _load_bsp_fail(&buffer, "failed to read header");

    // validate header
    if (!gs_string_compare_equal_n(map->header.magic, "IBSP", 4) || map->header.version != 46)
        return _load_bsp_fail(&buffer, "invalid header");

    // read entities lump
    if (!_load_entities_lump(&buffer, map))
        return _load_bsp_fail(&buffer, "failed to read entity lumps");

    // generic lumps
    void *containers[] = {
        &map->textures,
        &map->planes,
        &map->nodes,
        &map->leaves,
        &map->leaf_faces,
        &map->leaf_brushes,
        &map->models,
        &map->brushes,
        &map->brush_sides,
        &map->vertices,
        &map->mesh_verts,
        &map->effects,
        &map->faces,
        &map->lightmaps,
        &map->lightvols};

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
        sizeof(bsp_mesh_vert_lump_t),
        sizeof(bsp_effect_lump_t),
        sizeof(bsp_face_lump_t),
        sizeof(bsp_lightmap_lump_t),
        sizeof(bsp_lightvol_lump_t),
    };

    // read generics
    uint32_t start = BSP_LUMP_TYPE_TEXTURES;
    uint32_t end = BSP_LUMP_TYPE_LIGHTVOLS;
    for (size_t i = start; i <= end; i++)
    {
        if (!_load_lump(&buffer, map, containers[i - start], i, sizes[i - start]))
        {
            gs_println("load_bsp() failed: failed to read lump %d", i);
            gs_byte_buffer_free(&buffer);
            return false;
        }
    }

    // read visdata lump
    if (!_load_visdata_lump(&buffer, map))
        return _load_bsp_fail(&buffer, "failed to read visdata lump");

    gs_byte_buffer_free(&buffer);
    gs_println("load_bsp() done loading '%s'", filename);
    return true;
}

b32 _load_bsp_header(gs_byte_buffer_t *buffer, bsp_header_t *header)
{
    gs_byte_buffer_read(buffer, bsp_header_t, header);

    return true;
}

b32 _load_lump(gs_byte_buffer_t *buffer, bsp_map_t *map, void *container, bsp_lump_types type, uint32_t lump_size)
{
    int32_t size = map->header.dir_entries[type].length;

    int32_t *count = (int32_t)container;
    char **data = (char *)container + sizeof(int32_t);

    *count = size / lump_size;
    *data = gs_malloc(size);

    buffer->position = map->header.dir_entries[type].offset;
    gs_byte_buffer_read_bulk(buffer, data, size);

    return true;
}

b32 _load_entities_lump(gs_byte_buffer_t *buffer, bsp_map_t *map)
{
    int32_t size = map->header.dir_entries[BSP_LUMP_TYPE_ENTITIES].length;

    map->entities.ents = gs_malloc(size);

    buffer->position = map->header.dir_entries[BSP_LUMP_TYPE_ENTITIES].offset;
    gs_byte_buffer_read_bulk(buffer, &map->entities.ents, size);

    return true;
}

b32 _load_visdata_lump(gs_byte_buffer_t *buffer, bsp_map_t *map)
{
    buffer->position = map->header.dir_entries[BSP_LUMP_TYPE_VISDATA].offset;
    gs_byte_buffer_read(buffer, int32_t, &map->visdata.num_vecs);
    gs_byte_buffer_read(buffer, int32_t, &map->visdata.size_vecs);

    int32_t size = map->visdata.num_vecs * map->visdata.size_vecs;
    map->visdata.vecs = gs_malloc(size);
    gs_byte_buffer_read_bulk(buffer, &map->visdata.vecs, size);

    return true;
}

void free_bsp(bsp_map_t *map)
{
    if (map == NULL)
    {
        return;
    }

    gs_free(map->entities.ents);
    gs_free(map->textures.data);
    gs_free(map->planes.data);
    gs_free(map->nodes.data);
    gs_free(map->leaves.data);
    gs_free(map->leaf_faces.data);
    gs_free(map->leaf_brushes.data);
    gs_free(map->models.data);
    gs_free(map->brushes.data);
    gs_free(map->brush_sides.data);
    gs_free(map->vertices.data);
    gs_free(map->mesh_verts.data);
    gs_free(map->effects.data);
    gs_free(map->faces.data);
    gs_free(map->lightmaps.data);
    gs_free(map->lightvols.data);
    gs_free(map->visdata.vecs);

    map->entities.ents = NULL;
    map->textures.data = NULL;
    map->planes.data = NULL;
    map->nodes.data = NULL;
    map->leaves.data = NULL;
    map->leaf_faces.data = NULL;
    map->leaf_brushes.data = NULL;
    map->models.data = NULL;
    map->brushes.data = NULL;
    map->brush_sides.data = NULL;
    map->vertices.data = NULL;
    map->mesh_verts.data = NULL;
    map->effects.data = NULL;
    map->faces.data = NULL;
    map->lightmaps.data = NULL;
    map->lightvols.data = NULL;
    map->visdata.vecs = NULL;

    gs_free(map);
    map = NULL;
}