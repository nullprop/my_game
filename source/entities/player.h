/*================================================================
    * entities/player.h
    *
    * Copyright (c) 2021 Lauri Räsänen
    * ================================

    ...
=================================================================*/

#ifndef MG_PLAYER_H
#define MG_PLAYER_H

#include <gs/gs.h>

typedef struct mg_player_camera_t
{
    float32_t pitch;
    float32_t roll;
    gs_camera_t cam;
} mg_player_camera_t;

typedef struct mg_player_t
{
    gs_vqs transform;
    mg_player_camera_t camera;
    float32_t yaw;
    float32_t eye_height;
    int32_t health;
} mg_player_t;

#endif // MG_PLAYER_H