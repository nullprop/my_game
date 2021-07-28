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

    gs_println("1");
    gs_byte_buffer_t buffer = gs_byte_buffer_new();
    gs_byte_buffer_read_from_file(&buffer, filename);

    gs_println("2");
    // read header
    if (!_load_bsp_header(&buffer, &map->header))
        return _load_bsp_fail(&buffer, "failed to read header");

    gs_println("3");
    // validate header
    if (!gs_string_compare_equal_n(map->header.magic, "IBSP", 4) || map->header.version != 46)
        return _load_bsp_fail(&buffer, "invalid header");

    gs_println("4");
    // read entities lump
    if (!_load_entities_lump(&buffer, map))
        return _load_bsp_fail(&buffer, "failed to read entity lumps");

    gs_println("ents: %s", map->entities.ents);

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

    gs_println("5");
    // read generics
    uint32_t start = BSP_LUMP_TYPE_TEXTURES;
    uint32_t end = BSP_LUMP_TYPE_LIGHTVOLS;
    for (size_t i = start; i <= end; i++)
    {
        gs_println("load generic: %d", i);
        if (!_load_lump(&buffer, map, containers[i - start], i, sizes[i - start]))
        {
            gs_println("load_bsp() failed: failed to read lump %d", i);
            gs_byte_buffer_free(&buffer);
            return false;
        }
    }

    gs_println("texture address0 %p", &map->textures);
    gs_println("texture address1 %p", map->textures.data);
    gs_println("texture address2 %p", (uintptr_t)map->textures.data);

    gs_println("6");
    // read visdata lump
    if (!_load_visdata_lump(&buffer, map))
        return _load_bsp_fail(&buffer, "failed to read visdata lump");

    gs_println("7");
    gs_println("p count: %zu", map->planes.count);
    // Flip coordinates to Y up
    for (size_t i = 0; i < map->planes.count; i++)
    {
        gs_println("plane %d", i);
        gs_println("x: %f, y: %f, z: %f", map->planes.data[i].normal.x, map->planes.data[i].normal.y, map->planes.data[i].normal.z);
        map->planes.data[i].normal = gs_v3(
            map->planes.data[i].normal.x,
            map->planes.data[i].normal.z,
            map->planes.data[i].normal.y);
    }
    gs_println("8");
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
    gs_println("9");
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
    gs_println("10");
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

        int32_t temp = map->faces.data[i].lm_origin[1];
        map->faces.data[i].lm_origin[1] = map->faces.data[i].lm_origin[1];
        map->faces.data[i].lm_origin[2] = temp;
    }
    gs_println("11");
    for (size_t i = 0; i < map->nodes.count; i++)
    {
        int32_t temp = map->nodes.data[i].mins[1];
        map->nodes.data[i].mins[1] = map->nodes.data[i].mins[1];
        map->nodes.data[i].mins[2] = temp;

        temp = map->nodes.data[i].maxs[1];
        map->nodes.data[i].maxs[1] = map->nodes.data[i].maxs[1];
        map->nodes.data[i].maxs[2] = temp;
    }
    gs_println("12");
    for (size_t i = 0; i < map->leaves.count; i++)
    {
        int32_t temp = map->leaves.data[i].mins[1];
        map->leaves.data[i].mins[1] = map->leaves.data[i].mins[1];
        map->leaves.data[i].mins[2] = temp;

        temp = map->leaves.data[i].maxs[1];
        map->leaves.data[i].maxs[1] = map->leaves.data[i].maxs[1];
        map->leaves.data[i].maxs[2] = temp;
    }

    gs_println("8");
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
    uint32_t size = map->header.dir_entries[type].length;
    gs_println("generic size: %zu", size);
    gs_println("generic lump_size: %zu", lump_size);

    uint32_t *count = (uint32_t *)container;
    void **data = (void **)((char *)container + sizeof(uint32_t));
    
    gs_println("before malloc");
    gs_println("data addr2: %p", *data);
    gs_println("map->textures.data: %p", map->textures.data);

    *count = size / lump_size;
    *data = calloc(*count, lump_size);
    gs_println("after malloc");
    gs_println("data addr2: %p", *data);
    gs_println("map->textures.data: %p", map->textures.data);

    buffer->position = map->header.dir_entries[type].offset;
    gs_byte_buffer_read_bulk(buffer, data, size);


    gs_println("after read");
    gs_println("data addr2: %p", *data);
    gs_println("map->textures.data: %p", map->textures.data);

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