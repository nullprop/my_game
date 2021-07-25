/*================================================================
    * bsp/bsp_map.c
    *
    * Copyright (c) 2021 Lauri Räsänen
    * Copyright (c) 2018 Krzysztof Kondrak
    *
    * See README.md for license.
    * ================================

    BSP rendering.
=================================================================*/

#include "bsp_patch.c"
#include "bsp_types.h"

void _bsp_load_entities(bsp_map_t *map);
void _bsp_load_textures(bsp_map_t *map);
void _bsp_load_lightmaps(bsp_map_t *map);
void _bsp_load_lightvols(bsp_map_t *map);
void _bsp_create_patch(bsp_map_t *map, bsp_face_lump_t face);

void bsp_map_init(bsp_map_t *map)
{
    if (map->faces.count == 0)
    {
        return;
    }

    // Init dynamic arrays
    gs_dyn_array_reserve(map->render_faces, map->faces.count);
    gs_dyn_array_reserve(map->visible_faces, 1);
    gs_dyn_array_reserve(map->patches, 1);

    // Load stuff
    _bsp_load_entities(map);
    _bsp_load_textures(map);
    _bsp_load_lightmaps(map);
    _bsp_load_lightvols(map);

    // Data agregators for vertex and index buffer creation
    gs_dyn_array(bsp_face_lump_t) face_data = gs_dyn_array_new(bsp_face_lump_t);
    gs_dyn_array(bsp_patch_t) patch_data = gs_dyn_array_new(bsp_patch_t);

    uint32_t face_array_idx = 0;
    uint32_t patch_array_idx = 0;

    // Create renderable faces and patches
    for (size_t i = 0; i < map->faces.count; i++)
    {
        bsp_face_renderable_t face = {
            .type = map->faces.data[i].type,
            .index = i};

        if (face.type == BSP_FACE_TYPE_PATCH)
        {
            _bsp_create_patch(map, map->faces.data[i]);
            patch_array_idx++;
        }
        else
        {
            gs_dyn_array_push(face_data, map->faces.data[i]);
            face_array_idx++;
        }

        gs_dyn_array_push(map->render_faces, face);
    }

    // Create buffers
    // TODO

    gs_dyn_array_free(face_data);
    face_data = NULL;
    gs_dyn_array_free(patch_data);
    patch_data = NULL;

    // Static stats
    map->stats.total_vertices = map->vertices.count;
    map->stats.total_faces = map->faces.count;
    map->stats.total_patches = patch_array_idx;
}

void _bsp_load_entities(bsp_map_t *map)
{
}

void _bsp_load_textures(bsp_map_t *map)
{
}

void _bsp_load_lightmaps(bsp_map_t *map)
{
}

void _bsp_load_lightvols(bsp_map_t *map)
{
}

void _bsp_create_patch(bsp_map_t *map, bsp_face_lump_t face)
{
}

void bsp_map_update(bsp_map_t *map)
{
}

void bsp_map_render(bsp_map_t *map)
{
}

void bsp_map_free(bsp_map_t *map)
{
    if (map == NULL)
    {
        return;
    }

    for (size_t i = 0; i < gs_dyn_array_size(map->patches); i++)
    {
        bsp_patch_free(&map->patches[i]);
    }
    gs_dyn_array_free(map->patches);
    gs_dyn_array_free(map->visible_faces);
    gs_dyn_array_free(map->render_faces);

    map->patches = NULL;
    map->visible_faces = NULL;
    map->render_faces = NULL;

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