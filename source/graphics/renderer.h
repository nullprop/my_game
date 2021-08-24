/*================================================================
    * graphics/renderer.h
    *
    * Copyright (c) 2021 Lauri Räsänen
    * ================================

    ...
=================================================================*/

#ifndef MG_RENDERER_H
#define MG_RENDERER_H

#include <gs/gs.h>
#include <gs/util/gs_idraw.h>

#include "../bsp/bsp_map.h"
#include "../entities/player.h"
#include "model_manager.h"
#include "types.h"

typedef struct mg_renderable_t
{
    gs_vqs *transform;
    gs_mat4 u_view;
    mg_model_t model;
} mg_renderable_t;

typedef struct mg_renderer_t
{
    gs_command_buffer_t cb;
    gs_immediate_draw_t gsi;
    bool32_t use_immediate_mode;
    gs_camera_t *cam;
    bsp_map_t *bsp;
    mg_player_t *player;
    gs_slot_array(mg_renderable_t) renderables;
    gs_handle(gs_graphics_pipeline_t) pipe;
    gs_dyn_array(gs_handle(gs_graphics_shader_t)) shaders;
    gs_dyn_array(char *) shader_names;
    gs_handle(gs_graphics_uniform_t) u_proj;
    gs_handle(gs_graphics_uniform_t) u_view;
    gs_handle(gs_graphics_uniform_t) u_light;
    gs_handle(gs_graphics_uniform_t) u_tex;
    gs_handle(gs_graphics_texture_t) missing_texture;
} mg_renderer_t;

void mg_renderer_init();
void mg_renderer_update();
void mg_renderer_free();
uint32_t mg_renderer_create_renderable(mg_model_t model, gs_vqs *transform);
void mg_renderer_remove_renderable(uint32_t renderable_id);
mg_renderable_t *mg_renderer_get_renderable(uint32_t renderable_id);
gs_handle(gs_graphics_shader_t) mg_renderer_get_shader(char *name);
void _mg_renderer_renderable_pass(gs_vec2 fb);
void _mg_renderer_immediate_pass(gs_vec2 fb);
void _mg_renderer_draw_debug_overlay();
void _mg_renderer_load_shader(char *name);

extern mg_renderer_t *g_renderer;

#endif // MG_RENDERER_H