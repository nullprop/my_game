/*================================================================
    * graphics/types.h
    *
    * Copyright (c) 2021 Lauri Räsänen
    * ================================

    ...
=================================================================*/

#ifndef MG_GFX_TYPES_H
#define MG_GFX_TYPES_H

#include <gs/gs.h>

typedef struct mg_renderer_light_t
{
    gs_vec3 ambient;
    gs_vec3 directional;
    gs_vec3 direction;
} mg_renderer_light_t;

#endif // MG_GFX_TYPES_H