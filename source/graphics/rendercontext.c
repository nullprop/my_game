/*================================================================
    * graphics/rendercontext.c
    *
    * Copyright (c) 2021 Lauri Räsänen
    * ================================

    ...
=================================================================*/

#include "rendercontext.h"

mg_render_ctx *g_render_ctx;

void mg_render_ctx_init()
{
    // Allocate
    g_render_ctx = gs_malloc_init(mg_render_ctx);
    g_render_ctx->render_ctx_cb = gs_command_buffer_new();
    g_render_ctx->render_ctx_gsi = gs_immediate_draw_new();
}

void mg_render_ctx_update()
{
    if (g_render_ctx->render_ctx_use_immediate_mode)
    {
        //gsi_render_pass_submit(&render_ctx_gsi, &render_ctx_cb, gs_color(0, 0, 0, 0));
        // custom impl without clear
        gs_renderpass pass = gs_default_val();
        gs_vec2 fb = gs_platform_framebuffer_sizev(gs_platform_main_window());
        gs_graphics_begin_render_pass(&g_render_ctx->render_ctx_cb, pass);
        gs_graphics_set_viewport(&g_render_ctx->render_ctx_cb, 0, 0, (int32_t)fb.x, (int32_t)fb.y);
        gsi_draw(&g_render_ctx->render_ctx_gsi, &g_render_ctx->render_ctx_cb);
        gs_graphics_end_render_pass(&g_render_ctx->render_ctx_cb);
    }

    // Submit command buffer (syncs to GPU, MUST be done on main thread where you have your GPU context created)
    gs_graphics_submit_command_buffer(&g_render_ctx->render_ctx_cb);
}

void mg_render_ctx_free()
{
    gs_immediate_draw_free(&g_render_ctx->render_ctx_gsi);
    gs_command_buffer_free(&g_render_ctx->render_ctx_cb);
}