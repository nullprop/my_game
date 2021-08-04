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

static gs_command_buffer_t render_ctx_cb = {0};
static gs_immediate_draw_t render_ctx_gsi = {0};
static bool32_t render_ctx_use_immediate_mode = false;

void render_ctx_init();
void render_ctx_update();
void render_ctx_free();

#endif MG_RENDERCONTEXT_H