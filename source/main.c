/*================================================================
    * Copyright: 2021 Lauri Räsänen
    * main.c

    The main entry point of my_game.
=================================================================*/

#define GS_IMPL
#include <gs/gs.h>

#include "graphics/rendercontext.c"
#include "data.c"
#include "bsp/types.h"
#include "bsp/bsploader.c"

typedef struct fps_camera_t
{
    float pitch;
    float bob_time;
    gs_camera_t cam;
} fps_camera_t;

void fps_camera_update(fps_camera_t *cam);

fps_camera_t fps = {0};

void app_init()
{
    // Construct camera
    fps.cam = gs_camera_perspective();
    fps.cam.transform.position = gs_v3(0.f, 0.f, 0.f);

    // Lock mouse at start by default
    gs_platform_lock_mouse(gs_platform_main_window(), true);

    bsp_map_t* map;
    load_bsp("assets/maps/q3dm1.bsp", map);
    free_bsp(map);
    gs_println("map freed");

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

    render_ctx_update();
}

void fps_camera_update(fps_camera_t *fps)
{
    gs_platform_t *platform = gs_engine_subsystem(platform);

    gs_vec2 dp = gs_vec2_scale(gs_platform_mouse_deltav(), SENSITIVITY);
    const float mod = gs_platform_key_down(GS_KEYCODE_LEFT_SHIFT) ? 2.f : 1.f;
    float dt = platform->time.delta;
    float old_pitch = fps->pitch;

    // Keep track of previous amount to clamp the camera's orientation
    fps->pitch = gs_clamp(old_pitch + dp.y, -90.f, 90.f);

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

    // For a non-flying first person camera, need to lock the y movement velocity
    vel.y = 0.f;

    fps->cam.transform.position = gs_vec3_add(fps->cam.transform.position, gs_vec3_scale(gs_vec3_norm(vel), dt * CAM_SPEED * mod));

    // If moved, then we'll "bob" the camera some
    if (gs_vec3_len(vel) != 0.f)
    {
        fps->bob_time += dt * 8.f;
        float sb = sin(fps->bob_time);
        float bob_amt = (sb * 0.5f + 0.5f) * 0.1f * mod;
        float rot_amt = sb * 0.0004f * mod;
        fps->cam.transform.position.y = 2.f + bob_amt;
        fps->cam.transform.rotation = gs_quat_mul(fps->cam.transform.rotation, gs_quat_angle_axis(rot_amt, GS_ZAXIS));
    }
}

gs_app_desc_t gs_main(int32_t argc, char **argv)
{
    return (gs_app_desc_t){
        .init = app_init,
        .update = app_update};
}
