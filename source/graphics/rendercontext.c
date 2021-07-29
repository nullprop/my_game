/*================================================================
    * graphics/rendercontext.c
    *
    * Copyright (c) 2021 Lauri Räsänen
    * ================================

    ...
=================================================================*/

#include <gs/gs.h>
#include <gs/util/gs_idraw.h>

static gs_command_buffer_t *render_ctx_cb = NULL;
static gs_immediate_draw_t *render_ctx_gsi = NULL;

void render_ctx_init()
{
    render_ctx_cb = gs_malloc_init(gs_command_buffer_t);
    render_ctx_gsi = gs_malloc_init(gs_immediate_draw_t);

    // Construct new command buffer
    *render_ctx_cb = gs_command_buffer_new();
    *render_ctx_gsi = gs_immediate_draw_new();
}

void render_ctx_update()
{
    gsi_render_pass_submit(render_ctx_gsi, render_ctx_cb, gs_color(255, 0, 0, 255));

    // Submit command buffer (syncs to GPU, MUST be done on main thread where you have your GPU context created)
    gs_graphics_submit_command_buffer(render_ctx_cb);
}

void render_ctx_free()
{
    gs_immediate_draw_free(render_ctx_gsi);
    gs_command_buffer_free(render_ctx_cb);

    free(render_ctx_gsi);
    free(render_ctx_cb);

    render_ctx_gsi = NULL;
    render_ctx_cb = NULL;
}

gs_command_buffer_t *render_ctx_get_cb()
{
    return render_ctx_cb;
}

gs_immediate_draw_t *render_ctx_get_gsi()
{
    return render_ctx_gsi;
}