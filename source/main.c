/*================================================================
    * main.c
    *
    * Copyright (c) 2021 Lauri Räsänen
    * ================================

    The main entry point of my_game.
=================================================================*/

#define GS_IMPL
#include <gs/gs.h>
#define GS_IMMEDIATE_DRAW_IMPL
#include <gs/util/gs_idraw.h>
#define GS_GFXT_IMPL
#include <gs/util/gs_gfxt.h>

#include "audio/audio_manager.h"
#include "bsp/bsp_loader.h"
#include "bsp/bsp_map.h"
#include "entities/player.h"
#include "graphics/rendercontext.h"
#include "model/model_manager.h"
#include "util/config.h"

bsp_map_t *bsp_map = NULL;
mg_player_t *player = NULL;
gs_immediate_draw_t *gsi = NULL;

void app_spawn()
{
    if (bsp_map->valid)
    {
        player->velocity = gs_v3(0, 0, 0);
        player->camera.pitch = 0;
        bsp_map_find_spawn_point(bsp_map, &player->transform.position, &player->yaw);
        player->last_valid_pos = player->transform.position;
        player->yaw -= 90;
    }
}

void on_window_resize(GLFWwindow *window, int width, int height)
{
    if (player != NULL)
    {
        player->camera.cam.aspect_ratio = width / height;
    }
}

void app_init()
{
    glfwSetWindowSizeCallback(gs_platform_raw_window_handle(gs_platform_main_window()), &on_window_resize);

    mg_audio_manager_init();
    mg_model_manager_init();
    mg_render_ctx_init();
    g_render_ctx->use_immediate_mode = true;
    gsi = &g_render_ctx->gsi;

    // Lock mouse at start by default
    //gs_platform_lock_mouse(gs_platform_main_window(), true);
    if (glfwRawMouseMotionSupported())
    {
        glfwSetInputMode(gs_platform_raw_window_handle(gs_platform_main_window()), GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);
    }

    bsp_map = gs_malloc_init(bsp_map_t);
    load_bsp("maps/q3dm1.bsp", bsp_map);

    if (bsp_map->valid)
    {
        bsp_map_init(bsp_map);
    }

    player = mg_player_new();
    player->map = bsp_map;
    player->camera.cam.fov = g_config->graphics.fov;
    app_spawn();
}

void app_update()
{
    if (gs_platform_key_pressed(GS_KEYCODE_ESC))
        gs_engine_quit();

    if (gs_platform_key_pressed(GS_KEYCODE_R))
        app_spawn();

    if (gs_platform_key_pressed(GS_KEYCODE_F1))
    {
        uint32_t main_window = gs_platform_main_window();

        // TODO: monitor size should probably be in the api
        GLFWvidmode *vid_mode = glfwGetVideoMode(glfwGetPrimaryMonitor());

        bool32_t want_fullscreen = !gs_platform_window_fullscreen(main_window);
        gs_platform_set_window_fullscreen(main_window, want_fullscreen);

        if (!want_fullscreen)
        {
            gs_platform_set_window_size(main_window, 800, 600);

            // Going back to windowed mode,
            // restore window to center of screen.
            gs_vec2 window_size = gs_platform_window_sizev(main_window);
            gs_vec2 monitor_size = gs_v2(vid_mode->width, vid_mode->height);

            // Set position
            gs_vec2 top_left = gs_vec2_scale(gs_vec2_sub(monitor_size, window_size), 0.5f);
            gs_platform_set_window_positionv(main_window, top_left);
        }
        else
        {
            // Set to fullscreen res
            gs_platform_set_window_size(main_window, vid_mode->width, vid_mode->height);
        }
    }

    // If click, then lock again (in case lost)
    if (gs_platform_mouse_pressed(GS_MOUSE_LBUTTON) && !gs_platform_mouse_locked())
    {
        gs_platform_lock_mouse(gs_platform_main_window(), true);
        if (glfwRawMouseMotionSupported())
        {
            glfwSetInputMode(gs_platform_raw_window_handle(gs_platform_main_window()), GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);
        }
    }

    // Update player
    mg_player_update(player);

    // Update and render map
    if (bsp_map->valid)
    {
        bsp_map_update(bsp_map, player->camera.cam.transform.position);
        //bsp_map_render_immediate(bsp_map, gsi, &player->camera.cam);
        bsp_map_render(bsp_map, &player->camera.cam);
    }

    // Render viewmodel after map
    mg_player_render(player);

    // draw fps
    char temp[64];
    sprintf(temp, "fps: %d", (int)gs_round(1.0f / gs_platform_delta_time()));
    gsi_camera2D(gsi);
    gsi_text(gsi, 5, 15, temp, NULL, false, 255, 255, 255, 255);

    // draw map stats
    sprintf(temp, "map: %s", bsp_map->name);
    gsi_text(gsi, 5, 30, temp, NULL, false, 255, 255, 255, 255);
    sprintf(temp, "tris: %zu/%zu", bsp_map->stats.visible_indices / 3, bsp_map->stats.total_indices / 3);
    gsi_text(gsi, 10, 45, temp, NULL, false, 255, 255, 255, 255);
    sprintf(temp, "faces: %zu/%zu", bsp_map->stats.visible_faces, bsp_map->stats.total_faces);
    gsi_text(gsi, 10, 60, temp, NULL, false, 255, 255, 255, 255);
    sprintf(temp, "patches: %zu/%zu", bsp_map->stats.visible_patches, bsp_map->stats.total_patches);
    gsi_text(gsi, 10, 75, temp, NULL, false, 255, 255, 255, 255);
    sprintf(temp, "leaf: %zu, cluster: %d", bsp_map->stats.current_leaf, bsp_map->leaves.data[bsp_map->stats.current_leaf].cluster);
    gsi_text(gsi, 10, 90, temp, NULL, false, 255, 255, 255, 255);

    // draw player stats
    gsi_text(gsi, 5, 105, "player:", NULL, false, 255, 255, 255, 255);
    sprintf(temp, "pos: [%f, %f, %f]", player->transform.position.x, player->transform.position.y, player->transform.position.z);
    gsi_text(gsi, 10, 120, temp, NULL, false, 255, 255, 255, 255);
    sprintf(temp, "ang: [%f, %f, %f]", player->yaw, player->camera.pitch, player->camera.roll);
    gsi_text(gsi, 10, 135, temp, NULL, false, 255, 255, 255, 255);
    sprintf(temp, "vel: [%f, %f, %f]", player->velocity.x, player->velocity.y, player->velocity.z);
    gsi_text(gsi, 10, 150, temp, NULL, false, 255, 255, 255, 255);
    sprintf(temp, "vel_abs: %f, h: %f", gs_vec3_len(player->velocity), gs_vec3_len(gs_v3(player->velocity.x, player->velocity.y, 0)));
    gsi_text(gsi, 10, 165, temp, NULL, false, 255, 255, 255, 255);

    mg_render_ctx_update();
}

void app_shutdown()
{
    mg_player_free(player);
    bsp_map_free(bsp_map);
    mg_render_ctx_free();
    mg_audio_manager_free();
    mg_config_free();
}

gs_app_desc_t gs_main(int32_t argc, char **argv)
{
    // Load config first so we can use resolution, etc.
    mg_config_init();

    return (gs_app_desc_t){
        .init = app_init,
        .update = app_update,
        .shutdown = app_shutdown,
        .window_flags = g_config->video.fullscreen ? GS_WINDOW_FLAGS_FULLSCREEN : 0,
        .window_width = g_config->video.width,
        .window_height = g_config->video.height,
        .enable_vsync = g_config->video.vsync,
        .frame_rate = g_config->video.max_fps,
    };
}