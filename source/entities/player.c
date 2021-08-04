/*================================================================
    * entities/player.c
    *
    * Copyright (c) 2021 Lauri Räsänen
    * ================================

    ...
=================================================================*/

#include <gs/gs.h>

#include "../util/transform.h"
#include "player.h"

mg_player_t *mg_player_new()
{
    mg_player_t *player = gs_malloc_init(mg_player_t);
    *player = (mg_player_t){
        .transform = gs_vqs_default(),
        .camera = {
            .cam = {
                .aspect_ratio = 4 / 3,
                .far_plane = 3000.0f,
                .fov = 110.0f,
                .near_plane = 0.1f,
                .proj_type = GS_PROJECTION_TYPE_PERSPECTIVE,
            },
            .pitch = 0.0f,
            .roll = 0.0f,
        },
        .eye_height = 48.0f,
        .health = 100,
    };

    player->camera.cam.transform = gs_vqs_absolute_transform(
        &(gs_vqs){
            .position = gs_v3(0, 0, player->eye_height),
            .rotation = gs_quat_default(),
            .scale = gs_v3(1.0f, 1.0f, 1.0f),
        },
        &player->transform);

    return player;
}

void mg_player_free(mg_player_t *player)
{
    gs_free(player);
    player = NULL;
}

void mg_player_update(mg_player_t *player)
{
    gs_platform_t *platform = gs_engine_subsystem(platform);
    gs_vec2 dp = gs_vec2_scale(gs_platform_mouse_deltav(), 0.22f);
    float dt = platform->time.delta;

    // Rotate
    player->camera.pitch = gs_clamp(player->camera.pitch + dp.y, -90.0f, 90.0f);
    player->yaw = fmodf(player->yaw - dp.x, 360.0f);
    player->transform.rotation = gs_quat_angle_axis(gs_deg2rad(player->yaw), gs_absolute_up);
    _mg_player_camera_update(player);

    // Translate
    const float mod = gs_platform_key_down(GS_KEYCODE_LEFT_SHIFT) ? 5.0f : 1.0f;
    gs_vec3 vel = {0};
    if (gs_platform_key_down(GS_KEYCODE_W))
        vel = gs_vec3_add(vel, mg_get_forward(player->transform.rotation));
    if (gs_platform_key_down(GS_KEYCODE_S))
        vel = gs_vec3_add(vel, mg_get_backward(player->transform.rotation));
    if (gs_platform_key_down(GS_KEYCODE_D))
        vel = gs_vec3_add(vel, mg_get_right(player->transform.rotation));
    if (gs_platform_key_down(GS_KEYCODE_A))
        vel = gs_vec3_add(vel, mg_get_left(player->transform.rotation));

    player->transform.position = gs_vec3_add(player->transform.position, gs_vec3_scale(gs_vec3_norm(vel), dt * 320.0f * mod));
}

void _mg_player_camera_update(mg_player_t *player)
{
    player->camera.cam.transform = gs_vqs_absolute_transform(
        &(gs_vqs){
            .position = gs_v3(0, 0, player->eye_height),
            .rotation = gs_quat_mul(
                gs_quat_angle_axis(gs_deg2rad(-player->camera.pitch), gs_absolute_right),
                gs_quat_angle_axis(gs_deg2rad(player->camera.roll), gs_absolute_forward)),
            .scale = gs_v3(1.0f, 1.0f, 1.0f),
        },
        &player->transform);
}