/*================================================================
    * model/model.h
    *
    * Copyright (c) 2021 Lauri Räsänen
    * ================================

    Q3 MD3 version 15 loading.
=================================================================*/

#ifndef MODEL_H
#define MODEL_H

#include <gs/gs.h>

#define MD3_MAGIC "IDP3"
#define MD3_VERSION 15
#define MD3_SCALE 64.0f

typedef struct md3_header_t
{
    char magic[4];
    int32_t version;
    char name[64];
    int32_t flags;
    int32_t num_frames;
    int32_t num_tags;
    int32_t num_surfaces;
    int32_t num_skins;
    int32_t off_frames;
    int32_t off_tags;
    int32_t off_surfaces;
    int32_t off_end;
} md3_header_t;

typedef struct md3_frame_t
{
    gs_vec3 bounds_min;
    gs_vec3 bounds_max;
    gs_vec3 origin;
    float32_t radius;
    char name[16];
} md3_frame_t;

typedef struct md3_tag_t
{
    char name[64];
    gs_vec3 origin;
    gs_vec3 forward;
    gs_vec3 right;
    gs_vec3 up;
} md3_tag_t;

typedef struct md3_shader_t
{
    char name[64];
    int32_t index;
} md3_shader_t;

typedef struct md3_triangle_t
{
    int32_t indices[3];
} md3_triangle_t;

typedef struct md3_texcoord_t
{
    float32_t u;
    float32_t v;
} md3_texcoord_t;

typedef struct md3_vertex_t
{
    int16_t x;
    int16_t y;
    int16_t z;
    int16_t normal;
} md3_vertex_t;

// Helper struct used for rendering
typedef struct mg_md3_render_vertex_t
{
    gs_vec3 position;
    gs_vec3 normal;
    gs_vec2 texcoord;
} mg_md3_render_vertex_t;

typedef struct md3_surface_t
{
    char *magic;
    char *name;
    int32_t flags;
    int32_t num_frames;
    int32_t num_shaders;
    int32_t num_verts;
    int32_t num_tris;
    int32_t off_tris;
    int32_t off_shaders;
    int32_t off_texcoords;
    int32_t off_verts;
    int32_t off_end;
    md3_shader_t *shaders;
    md3_triangle_t *triangles;
    md3_texcoord_t *texcoords;
    md3_vertex_t *vertices;
    mg_md3_render_vertex_t *render_vertices;
    gs_handle_gs_graphics_vertex_buffer_t *vbos;
    gs_handle_gs_graphics_index_buffer_t ibo;
    gs_asset_texture_t *textures;
} md3_surface_t;

typedef struct md3_t
{
    md3_header_t header;
    md3_frame_t *frames;
    md3_tag_t *tags;
    md3_surface_t *surfaces;
} md3_t;

b32 _mg_load_md3_fail(gs_byte_buffer_t *buffer, char *msg);
bool mg_load_md3(char *filename, md3_t *model);
void mg_free_md3(md3_t *model);

#endif // MODEL_H