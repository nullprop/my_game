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

#include "../bsp/bsp_map.h"

#define MG_PLAYER_HEIGHT 64.0f
#define MG_PLAYER_CROUCH_HEIGHT 42.0f
#define MG_PLAYER_EYE_OFFSET 4.0f
#define MG_PLAYER_HALF_WIDTH 16.0f
#define MG_PLAYER_MOVE_SPEED 280.0f
#define MG_PLAYER_CROUCH_MOVE_SPEED 100.0f
#define MG_PLAYER_ACCEL 10.0f
#define MG_PLAYER_AIR_MOVE_SPEED 30.0f
#define MG_PLAYER_AIR_ACCEL 50.0f
#define MG_PLAYER_FRICTION 8.0f
#define MG_PLAYER_JUMP_SPEED 250.0f
#define MG_PLAYER_STOP_SPEED 100.0f
#define MG_PLAYER_MAX_VEL 3500.0f

typedef struct mg_player_camera_t
{
    float32_t pitch;
    float32_t roll;
    gs_camera_t cam;
} mg_player_camera_t;

typedef struct mg_player_t
{
    bsp_map_t *map; // TODO: remove
    gs_vqs transform;
    mg_player_camera_t camera;
    float32_t yaw;
    int32_t health;
    gs_vec3 velocity;
    gs_vec3 wish_move;
    gs_vec3 mins;
    gs_vec3 maxs;
    gs_vec3 eye_pos;
    gs_vec3 ground_normal;
    bool32_t wish_jump;
    bool32_t wish_crouch;
    bool32_t crouched;
    bool32_t grounded;
} mg_player_t;

mg_player_t *mg_player_new();
void mg_player_free(mg_player_t *player);
void mg_player_update(mg_player_t *player);
void _mg_player_unstuck(mg_player_t *player);
void _mg_player_slidemove(mg_player_t *player, float delta_time);
void _mg_player_uncrouch(mg_player_t *player, float delta_time);
void _mg_player_crouch(mg_player_t *player, float delta_time);
void _mg_player_accelerate(mg_player_t *player, float delta_time, float move_speed, float acceleration, float max_vel);
void _mg_player_friction(mg_player_t *player, float delta_time);
void _mg_player_camera_update(mg_player_t *player);
void _mg_player_get_input(mg_player_t *player, float delta_time);
void _mg_player_check_floor(mg_player_t *player);

#endif // MG_PLAYER_H