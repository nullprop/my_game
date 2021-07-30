/*================================================================
    * util/config.h
    *
    * Copyright (c) 2021 Lauri Räsänen
    * ================================

    Config types.
=================================================================*/

#ifndef MG_CONFIG_H
#define MG_CONFIG_H

#include <gs/gs.h>

typedef struct mg_config_video_t
{
    bool32_t fullscreen;
    uint32_t width;
    uint32_t height;
    uint32_t max_fps;
    bool32_t vsync;
} mg_config_video_t;

typedef struct mg_config_t
{
    mg_config_video_t video;
} mg_config_t;

#endif // MG_CONFIG_H