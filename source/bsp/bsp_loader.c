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

#include <gs/gs.h>

#include "bsp_types.h"

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
        &map->indices,
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
        sizeof(bsp_index_lump_t),
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

    // Flip coordinates to Y up
    for (size_t i = 0; i < map->planes.count; i++)
    {
        map->planes.data[i].normal = gs_v3(
            map->planes.data[i].normal.x,
            map->planes.data[i].normal.z,
            map->planes.data[i].normal.y);
    }
    for (size_t i = 0; i < map->models.count; i++)
    {
        map->models.data[i].mins = gs_v3(
            map->models.data[i].mins.x,
            map->models.data[i].mins.z,
            map->models.data[i].mins.y);
        map->models.data[i].maxs = gs_v3(
            map->models.data[i].maxs.x,
            map->models.data[i].maxs.z,
            map->models.data[i].maxs.y);
    }
    for (size_t i = 0; i < map->vertices.count; i++)
    {
        map->vertices.data[i].position = gs_v3(
            map->vertices.data[i].position.x,
            map->vertices.data[i].position.z,
            map->vertices.data[i].position.y);
        map->vertices.data[i].normal = gs_v3(
            map->vertices.data[i].normal.x,
            map->vertices.data[i].normal.z,
            map->vertices.data[i].normal.y);
    }
    for (size_t i = 0; i < map->faces.count; i++)
    {
        map->faces.data[i].normal = gs_v3(
            map->faces.data[i].normal.x,
            map->faces.data[i].normal.z,
            map->faces.data[i].normal.y);
        map->faces.data[i].lm_vecs[0] = gs_v3(
            map->faces.data[i].lm_vecs[0].x,
            map->faces.data[i].lm_vecs[0].z,
            map->faces.data[i].lm_vecs[0].y);
        map->faces.data[i].lm_vecs[1] = gs_v3(
            map->faces.data[i].lm_vecs[1].x,
            map->faces.data[i].lm_vecs[1].z,
            map->faces.data[i].lm_vecs[1].y);

        int32 temp = map->faces.data[i].lm_origin[1];
        map->faces.data[i].lm_origin[1] = map->faces.data[i].lm_origin[1];
        map->faces.data[i].lm_origin[2] = temp;
    }
    for (size_t i = 0; i < map->nodes.count; i++)
    {
        int32 temp = map->nodes.data[i].mins[1];
        map->nodes.data[i].mins[1] = map->nodes.data[i].mins[1];
        map->nodes.data[i].mins[2] = temp;

        temp = map->nodes.data[i].maxs[1];
        map->nodes.data[i].maxs[1] = map->nodes.data[i].maxs[1];
        map->nodes.data[i].maxs[2] = temp;
    }
    for (size_t i = 0; i < map->leaves.count; i++)
    {
        int32 temp = map->leaves.data[i].mins[1];
        map->leaves.data[i].mins[1] = map->leaves.data[i].mins[1];
        map->leaves.data[i].mins[2] = temp;

        temp = map->leaves.data[i].maxs[1];
        map->leaves.data[i].maxs[1] = map->leaves.data[i].maxs[1];
        map->leaves.data[i].maxs[2] = temp;
    }

    map->valid = true;
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
    void **data = (unsigned long long)container + sizeof(int32_t);

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