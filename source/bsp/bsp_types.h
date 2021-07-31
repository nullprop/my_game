/*================================================================
    * bsp/bsp_types.h
    *
    * Copyright (c) 2021 Lauri Räsänen
    * ================================

    BSP data types.

    Based on document by Kekoa Proudfoot:
    http://www.mralligator.com/q3/
=================================================================*/

#ifndef BSP_TYPES_H
#define BSP_TYPES_H

#include <gs/gs.h>

/*========
// ENUMS
=========*/

typedef enum bsp_content
{
    BSP_CONTENT_UNKNOWN = 0,
    BSP_CONTENT_CONTENTS_SOLID,
    BSP_CONTENT_COUNT
} bsp_content;

typedef enum bsp_face_type
{
    BSP_FACE_TYPE_POLYGON = 1,
    BSP_FACE_TYPE_PATCH,
    BSP_FACE_TYPE_MESH,
    BSP_FACE_TYPE_BILLBOARD,
    BSP_FACE_TYPE_COUNT
} bsp_face_type;

typedef enum bsp_render_flags
{
    SHOW_WIREFRAME = 1 << 0,
    SHOW_LIGHTMAPS = 1 << 1,
    USE_LIGHTMAPS = 1 << 2,
    ALPHA_TEST = 1 << 3,
    SKIP_MISSING_TEX = 1 << 4,
    SKIP_PVS = 1 << 5,
    SKIP_FC = 1 << 6,
    MULTISAMPLING = 1 << 7
} bsp_render_flags;

typedef enum bsp_lump_types
{
    BSP_LUMP_TYPE_ENTITIES = 0,
    BSP_LUMP_TYPE_TEXTURES,
    BSP_LUMP_TYPE_PLANES,
    BSP_LUMP_TYPE_NODES,
    BSP_LUMP_TYPE_LEAVES,
    BSP_LUMP_TYPE_LEAF_FACES,
    BSP_LUMP_TYPE_LEAF_BRUSHES,
    BSP_LUMP_TYPE_MODELS,
    BSP_LUMP_TYPE_BRUSHES,
    BSP_LUMP_TYPE_BRUSH_SIDES,
    BSP_LUMP_TYPE_VERTICES,
    BSP_LUMP_TYPE_INDICES,
    BSP_LUMP_TYPE_EFFECTS,
    BSP_LUMP_TYPE_FACES,
    BSP_LUMP_TYPE_LIGHTMAPS,
    BSP_LUMP_TYPE_LIGHTVOLS,
    BSP_LUMP_TYPE_VISDATA,
    BSP_LUMP_TYPE_COUNT
} bsp_lump_types;

/*==========
// STRUCTS
===========*/

typedef struct bsp_stats_t
{
    uint32_t total_vertices;
    uint32_t total_faces;
    uint32_t total_patches;
    uint32_t visible_faces;
    uint32_t visible_patches;
    uint32_t total_textures;
    uint32_t loaded_textures;
    uint32_t models;
    int32_t current_leaf;
} bsp_stats_t;

typedef struct bsp_face_renderable_t
{
    int32_t type;
    int32_t index;
} bsp_face_renderable_t;

/*
typedef struct bsp_leaf_renderable_t
{
    int32_t vis_cluster;
    int32_t first_face;
    int32_t num_faces;
    gs_vec3 mins;
    gs_vec3 maxs;
} bsp_leaf_renderable_t;
*/

typedef struct bsp_dir_entry_t
{
    int32_t offset;
    int32_t length;
} bsp_dir_entry_t;

typedef struct bsp_entity_lump_t
{
    char *ents;
} bsp_entity_lump_t;

typedef struct bsp_texture_lump_t
{
    char name[64];
    int32_t flags;
    int32_t contents;
} bsp_texture_lump_t;

typedef struct bsp_plane_lump_t
{
    gs_vec3 normal;
    float32_t dist;
} bsp_plane_lump_t;

typedef struct bsp_node_lump_t
{
    int32_t first_plane;
    int32_t children[2];
    int32_t mins[3];
    int32_t maxs[3];
} bsp_node_lump_t;

typedef struct bsp_leaf_lump_t
{
    int32_t cluster;
    int32_t area;
    int32_t mins[3];
    int32_t maxs[3];
    int32_t first_leaf_face;
    int32_t num_leaf_faces;
    int32_t first_leaf_brush;
    int32_t num_leaf_brushes;
} bsp_leaf_lump_t;

typedef struct bsp_leaf_face_lump_t
{
    int32_t face;
} bsp_leaf_face_lump_t;

typedef struct bsp_leaf_brush_lump_t
{
    int32_t brush;
} bsp_leaf_brush_lump_t;

typedef struct bsp_model_lump_t
{
    gs_vec3 mins;
    gs_vec3 maxs;
    int32_t first_face;
    int32_t num_faces;
    int32_t first_brush;
    int32_t num_brushes;
} bsp_model_lump_t;

typedef struct bsp_brush_lump_t
{
    int32_t first_brush_side;
    int32_t num_brush_sides;
    int32_t texture;
} bsp_brush_lump_t;

typedef struct bsp_brush_side_lump_t
{
    int32_t plane;
    int32_t texture;
} bsp_brush_side_lump_t;

typedef struct bsp_vert_lump_t
{
    gs_vec3 position;
    gs_vec2 tex_coord;
    gs_vec2 lm_coord;
    gs_vec3 normal;
    gs_color_t color;
} bsp_vert_lump_t;

typedef struct bsp_index_lump_t
{
    int32_t offset;
} bsp_index_lump_t;

typedef struct bsp_effect_lump_t
{
    char name[64];
    int32_t brush;
    int32_t unknown;
} bsp_effect_lump_t;

typedef struct bsp_face_lump_t
{
    int32_t texture;
    int32_t effect;
    int32_t type;
    int32_t first_vertex;
    int32_t num_vertices;
    int32_t first_index;
    int32_t num_indices;
    int32_t lm_index;
    //gs_vec2i lm_start;
    //gs_vec2i lm_size;
    //gs_vec2i lm_origin;
    int32_t lm_start[2];
    int32_t lm_size[2];
    int32_t lm_origin[3];
    gs_vec3 lm_vecs[2];
    gs_vec3 normal;
    //gs_vec2i size;
    int32_t size[2];
} bsp_face_lump_t;

typedef struct bsp_lightmap_lump_t
{
    char map[128 * 128 * 3];
} bsp_lightmap_lump_t;

typedef struct bsp_lightvol_lump_t
{
    char ambient[3];
    char directional[3];
    char dir[2];
} bsp_lightvol_lump_t;

typedef struct bsp_visdata_lump_t
{
    int32_t num_vecs;
    int32_t size_vecs;
    char *vecs;
} bsp_visdata_lump_t;

typedef struct bsp_quadratic_patch_t
{
    int32_t tesselation;
    bsp_vert_lump_t control_points[9];
    gs_dyn_array(bsp_vert_lump_t) vertices;
    gs_dyn_array(uint16_t) indices;
} bsp_quadratic_patch_t;

typedef struct bsp_patch_t
{
    int32_t texture_idx;
    int32_t lightmap_idx;
    int32_t width;
    int32_t height;
    gs_dyn_array(bsp_quadratic_patch_t) quadratic_patches;
} bsp_patch_t;

typedef struct bsp_header_t
{
    char magic[4];
    int32_t version;
    bsp_dir_entry_t dir_entries[BSP_LUMP_TYPE_COUNT];
} bsp_header_t;

typedef struct bsp_map_t
{
    /*==== File data ====*/

    bsp_header_t header;
    bsp_entity_lump_t entities;

    struct
    {
        uint32_t count;
        bsp_texture_lump_t *data;
    } textures;

    struct
    {
        uint32_t count;
        bsp_plane_lump_t *data;
    } planes;

    struct
    {
        uint32_t count;
        bsp_node_lump_t *data;
    } nodes;

    struct
    {
        uint32_t count;
        bsp_leaf_lump_t *data;
    } leaves;

    struct
    {
        uint32_t count;
        bsp_leaf_face_lump_t *data;
    } leaf_faces;

    struct
    {
        uint32_t count;
        bsp_leaf_brush_lump_t *data;
    } leaf_brushes;

    struct
    {
        uint32_t count;
        bsp_model_lump_t *data;
    } models;

    struct
    {
        uint32_t count;
        bsp_brush_lump_t *data;
    } brushes;

    struct
    {
        uint32_t count;
        bsp_brush_side_lump_t *data;
    } brush_sides;

    struct
    {
        uint32_t count;
        bsp_vert_lump_t *data;
    } vertices;

    struct
    {
        uint32_t count;
        bsp_index_lump_t *data;
    } indices;

    struct
    {
        uint32_t count;
        bsp_effect_lump_t *data;
    } effects;

    struct
    {
        uint32_t count;
        bsp_face_lump_t *data;
    } faces;

    struct
    {
        uint32_t count;
        bsp_lightmap_lump_t *data;
    } lightmaps;

    struct
    {
        uint32_t count;
        bsp_lightvol_lump_t *data;
    } lightvols;

    bsp_visdata_lump_t visdata;

    /*==== Runtime data ====*/

    char* name;
    bool32_t valid;
    bsp_stats_t stats;
    gs_dyn_array(bsp_patch_t) patches;
    gs_dyn_array(bsp_face_renderable_t) visible_faces;
    gs_dyn_array(bsp_face_renderable_t) render_faces;

    struct
    {
        uint32_t count;
        gs_asset_texture_t *data;
    } texture_assets;

    gs_handle(gs_graphics_texture_t) missing_texture;

    int32_t previous_leaf;
} bsp_map_t;

#endif // BSP_TYPES_H