/*================================================================
    * bsp/bsp_map.h
    *
    * Copyright (c) 2021 Lauri Räsänen
    * ================================

    ...
=================================================================*/

#ifndef BSP_MAP_H
#define BSP_MAP_H

#include <gs/gs.h>
#include <gs/util/gs_idraw.h>

#include "../util/math.h"
#include "bsp_entity.h"
#include "bsp_patch.h"
#include "bsp_shaders.h"
#include "bsp_types.h"

static gs_command_buffer_t bsp_graphics_cb = {0};
static gs_handle(gs_graphics_vertex_buffer_t) bsp_graphics_vbo = {0};
static gs_handle(gs_graphics_index_buffer_t) bsp_graphics_ibo = {0};
static gs_handle(gs_graphics_pipeline_t) bsp_graphics_pipe = {0};
static gs_handle(gs_graphics_shader_t) bsp_graphics_shader = {0};
static gs_handle(gs_graphics_uniform_t) bsp_graphics_u_proj = {0};
static gs_handle(gs_graphics_uniform_t) bsp_graphics_u_tex = {0};
static gs_handle(gs_graphics_uniform_t) bsp_graphics_u_lm = {0};
static gs_dyn_array(uint32_t) bsp_graphics_index_arr;
static gs_dyn_array(bsp_vert_lump_t) bsp_graphics_vert_arr;

void bsp_map_init(bsp_map_t *map);
void _bsp_load_entities(bsp_map_t *map);
void _bsp_load_textures(bsp_map_t *map);
void _bsp_load_lightmaps(bsp_map_t *map);
void _bsp_load_lightvols(bsp_map_t *map);
void _bsp_create_patch(bsp_map_t *map, bsp_face_lump_t face);
void _bsp_map_create_buffers(bsp_map_t *map);
void bsp_map_update(bsp_map_t *map, gs_vec3 view_position);
void bsp_map_render_immediate(bsp_map_t *map, gs_immediate_draw_t *gsi, gs_camera_t *cam);
void bsp_map_render(bsp_map_t *map, gs_camera_t *cam);
gs_vec3 bsp_map_find_spawn_point(bsp_map_t *map, gs_vec3 *position, float32_t *yaw);
void bsp_map_free(bsp_map_t *map);
int32_t _bsp_find_camera_leaf(bsp_map_t *map, gs_vec3 view_position);
void _bsp_calculate_visible_faces(bsp_map_t *map, int32_t leaf);
bool32_t _bsp_cluster_visible(bsp_map_t *map, int32_t view_cluster, int32_t test_cluster);

#endif // BSP_MAP_H