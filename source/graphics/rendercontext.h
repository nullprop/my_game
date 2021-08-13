/*================================================================
    * graphics/rendercontext.h
    *
    * Copyright (c) 2021 Lauri Räsänen
    * ================================

    ...
=================================================================*/

#ifndef MG_RENDERCONTEXT_H
#define MG_RENDERCONTEXT_H

#include <gs/gs.h>
#include <gs/util/gs_idraw.h>

typedef struct mg_render_ctx
{
    gs_command_buffer_t render_ctx_cb;
    gs_immediate_draw_t render_ctx_gsi;
    bool32_t render_ctx_use_immediate_mode;
} mg_render_ctx;

void mg_render_ctx_init();
void mg_render_ctx_update();
void mg_render_ctx_free();

extern mg_render_ctx *g_render_ctx;

#endif MG_RENDERCONTEXT_H