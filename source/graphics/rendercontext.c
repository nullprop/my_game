/*================================================================
    * graphics/rendercontext.c
    *
    * Copyright (c) 2021 Lauri Räsänen
    * ================================

    ...
=================================================================*/

#include <gs/gs.h>

gs_command_buffer_t cb = {0};

void render_ctx_init()
{
    // Construct new command buffer
    cb = gs_command_buffer_new();
}

void render_ctx_update()
{
    // Submit command buffer (syncs to GPU, MUST be done on main thread where you have your GPU context created)
    gs_graphics_submit_command_buffer(&cb);
}