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

void render_ctx_init();
void render_ctx_update();
void render_ctx_free();
gs_command_buffer_t *render_ctx_get_cb();
gs_immediate_draw_t *render_ctx_get_gsi();
void render_ctx_set_use_immediate_mode(bool32_t value);

#endif MG_RENDERCONTEXT_H