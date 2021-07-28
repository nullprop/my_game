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

#include "bsp/bsp_loader.c"
#include "bsp/bsp_map.c"
#include "graphics/rendercontext.c"

typedef struct fps_camera_t
{
    float pitch;
    float roll;
    float yaw;
    float bob_time;
    gs_camera_t cam;
} fps_camera_t;

void fps_camera_update(fps_camera_t *cam);

fps_camera_t fps = {0};

bsp_map_t *bsp_map = NULL;

void app_init()
{
    // Construct camera
    fps.cam = gs_camera_perspective();
    fps.cam.transform.position = gs_v3(0.f, 0.f, 0.f);
    fps.cam.fov = 90.0f;

    // Lock mouse at start by default
    //gs_platform_lock_mouse(gs_platform_main_window(), true);

    bsp_map = gs_malloc_init(bsp_map_t);
    load_bsp("maps/q3dm1.bsp", bsp_map);

    if (bsp_map->valid)
    {
        bsp_map_init(bsp_map);
        gs_println("map stats:");
        gs_println("  vertices: %d", bsp_map->stats.total_vertices);
        gs_println("  faces: %d", bsp_map->stats.total_faces);
        gs_println("  patches: %d", bsp_map->stats.total_patches);
        gs_println("  textures: %d/%d", bsp_map->stats.loaded_textures, bsp_map->stats.total_textures);
    }

    render_ctx_init();
}

void app_update()
{
    if (gs_platform_key_pressed(GS_KEYCODE_ESC))
        gs_engine_quit();

    // If click, then lock again (in case lost)
    if (gs_platform_mouse_pressed(GS_MOUSE_LBUTTON) && !gs_platform_mouse_locked())
    {
        fps.cam.transform.rotation = gs_quat_default();
        fps.pitch = 0.f;
        gs_platform_lock_mouse(gs_platform_main_window(), true);
    }

    // Update camera
    if (gs_platform_mouse_locked())
    {
        fps_camera_update(&fps);
    }

    if (bsp_map->valid)
    {
        bsp_map_update(bsp_map);
        bsp_map_render(bsp_map, render_ctx_get_gsi(), &fps.cam);
    }

    render_ctx_update();
}

void fps_camera_update(fps_camera_t *fps)
{
    gs_platform_t *platform = gs_engine_subsystem(platform);

    gs_vec2 dp = gs_vec2_scale(gs_platform_mouse_deltav(), 0.22f);
    const float mod = gs_platform_key_down(GS_KEYCODE_LEFT_SHIFT) ? 5.0f : 1.0f;
    float dt = platform->time.delta;
    float old_pitch = fps->pitch;
    float old_yaw = fps->yaw;

    // Keep track of previous amount to clamp the camera's orientation
    fps->pitch = gs_clamp(old_pitch + dp.y, -90.0f, 90.0f);
    fps->yaw = fmodf(old_yaw - dp.x, 360.0f);

    // Rotate camera
    gs_camera_offset_orientation(&fps->cam, -dp.x, old_pitch - fps->pitch);

    gs_vec3 vel = {0};
    if (gs_platform_key_down(GS_KEYCODE_W))
        vel = gs_vec3_add(vel, gs_camera_forward(&fps->cam));
    if (gs_platform_key_down(GS_KEYCODE_S))
        vel = gs_vec3_add(vel, gs_camera_backward(&fps->cam));
    if (gs_platform_key_down(GS_KEYCODE_A))
        vel = gs_vec3_add(vel, gs_camera_left(&fps->cam));
    if (gs_platform_key_down(GS_KEYCODE_D))
        vel = gs_vec3_add(vel, gs_camera_right(&fps->cam));

    fps->cam.transform.position = gs_vec3_add(fps->cam.transform.position, gs_vec3_scale(gs_vec3_norm(vel), dt * 320.0f * mod));
}

void app_shutdown()
{
    bsp_map_free(bsp_map);
}

gs_app_desc_t gs_main(int32_t argc, char **argv)
{
    return (gs_app_desc_t){
        .init = app_init,
        .update = app_update,
        .shutdown = app_shutdown};
}
