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

#include "model_manager.h"

typedef struct mg_renderable_t
{
    gs_vqs *transform;
    mg_model_t model;
} mg_renderable_t;

typedef struct mg_renderer_t
{
    gs_command_buffer_t cb;
    gs_immediate_draw_t gsi;
    bool32_t use_immediate_mode;
    gs_camera_t *cam;
    gs_slot_array(mg_renderable_t) renderables;
    gs_handle(gs_graphics_pipeline_t) pipe;
    gs_handle(gs_graphics_shader_t) shader;
    gs_handle(gs_graphics_uniform_t) u_proj;
} mg_renderer_t;

void mg_renderer_init();
void mg_renderer_update();
void mg_renderer_free();
uint32_t mg_renderer_create_renderable(mg_model_t model, gs_vqs *transform);
void mg_renderer_remove_renderable(uint32_t renderable_id);
mg_renderable_t *mg_renderer_get_renderable(uint32_t renderable_id);
void _mg_renderer_renderable_pass(gs_vec2 fb);
void _mg_renderer_immediate_pass(gs_vec2 fb);

extern mg_renderer_t *g_renderer;

#endif // MG_RENDERER_H