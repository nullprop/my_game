/*================================================================
    * graphics/rendercontext.c
    *
    * Copyright (c) 2021 Lauri Räsänen
    * ================================

    ...
=================================================================*/

#include "rendercontext.h"

static gs_command_buffer_t render_ctx_cb = {0};
static gs_immediate_draw_t render_ctx_gsi = {0};
static bool32_t render_ctx_use_immediate_mode = false;

void render_ctx_init()
{
    // Construct
    render_ctx_cb = gs_command_buffer_new();
    render_ctx_gsi = gs_immediate_draw_new();
}

void render_ctx_update()
{
    if (render_ctx_use_immediate_mode)
    {
        //gsi_render_pass_submit(&render_ctx_gsi, &render_ctx_cb, gs_color(0, 0, 0, 0));
        // custom impl without clear
        gs_renderpass pass = gs_default_val();
        gs_vec2 fb = gs_platform_framebuffer_sizev(gs_platform_main_window());
        gs_graphics_begin_render_pass(&render_ctx_cb, pass);
        gs_graphics_set_viewport(&render_ctx_cb, 0, 0, (int32_t)fb.x, (int32_t)fb.y);
        gsi_draw(&render_ctx_gsi, &render_ctx_cb);
        gs_graphics_end_render_pass(&render_ctx_cb);
    }

    // Submit command buffer (syncs to GPU, MUST be done on main thread where you have your GPU context created)
    gs_graphics_submit_command_buffer(&render_ctx_cb);
}

void render_ctx_free()
{
    gs_immediate_draw_free(&render_ctx_gsi);
    gs_command_buffer_free(&render_ctx_cb);
}

gs_command_buffer_t *render_ctx_get_cb()
{
    return &render_ctx_cb;
}

gs_immediate_draw_t *render_ctx_get_gsi()
{
    return &render_ctx_gsi;
}

void render_ctx_set_use_immediate_mode(bool32_t value)
{
    render_ctx_use_immediate_mode = value;
}