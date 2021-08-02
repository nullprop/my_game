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
#include "util/config.c"

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

void z_up()
{
    // Orient coordinate system
    gs_absolute_up = gs_v3(0, 0, 1.0f);
    gs_absolute_forward = gs_v3(0, 1.0f, 0);
    gs_absolute_right = gs_v3(1.0f, 0, 0);
}

void y_up()
{
    // Orient coordinate system
    gs_absolute_up = gs_v3(0, 1.0f, 0);
    gs_absolute_forward = gs_v3(0, 0, -1.0f);
    gs_absolute_right = gs_v3(1.0f, 0, 0);
}

void app_init()
{
    z_up();

    render_ctx_init();
    render_ctx_use_immediate_mode = true;

    // Construct camera
    fps.cam = gs_camera_perspective();
    fps.cam.transform.position = gs_v3(0.f, 0.f, 0.f);
    fps.cam.fov = 110.0f;
    fps.cam.far_plane = 2000.0f;

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
}

void app_update()
{
    if (gs_platform_key_pressed(GS_KEYCODE_ESC))
        gs_engine_quit();

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
        fps.cam.transform.rotation = gs_quat_default();
        fps.pitch = 0.f;
        gs_platform_lock_mouse(gs_platform_main_window(), true);
        if (glfwRawMouseMotionSupported())
        {
            glfwSetInputMode(gs_platform_raw_window_handle(gs_platform_main_window()), GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);
        }
    }

    // Update camera
    if (gs_platform_mouse_locked())
    {
        fps_camera_update(&fps);
    }

    if (bsp_map->valid)
    {
        bsp_map_update(bsp_map, fps.cam.transform.position);
        //bsp_map_render_immediate(bsp_map, &render_ctx_gsi, &fps.cam);
        bsp_map_render(bsp_map, &fps.cam);
    }

    // draw fps
    y_up();
    char temp[64];
    sprintf(temp, "fps: %d", (int)gs_round(1.0f / gs_platform_delta_time()));
    gsi_camera2D(&render_ctx_gsi);
    gsi_text(&render_ctx_gsi, 5, 15, temp, NULL, false, 255, 255, 255, 255);

    // draw map stats
    sprintf(temp, "map: %s", bsp_map->name);
    gsi_text(&render_ctx_gsi, 5, 30, temp, NULL, false, 255, 255, 255, 255);
    sprintf(temp, "tris: %zu/%zu", bsp_map->stats.visible_indices / 3, bsp_map->stats.total_indices / 3);
    gsi_text(&render_ctx_gsi, 10, 45, temp, NULL, false, 255, 255, 255, 255);
    sprintf(temp, "faces: %zu/%zu", bsp_map->stats.visible_faces, bsp_map->stats.total_faces);
    gsi_text(&render_ctx_gsi, 10, 60, temp, NULL, false, 255, 255, 255, 255);
    sprintf(temp, "patches: %zu/%zu", bsp_map->stats.visible_patches, bsp_map->stats.total_patches);
    gsi_text(&render_ctx_gsi, 10, 75, temp, NULL, false, 255, 255, 255, 255);
    sprintf(temp, "leaf: %zu", bsp_map->stats.current_leaf);
    gsi_text(&render_ctx_gsi, 10, 90, temp, NULL, false, 255, 255, 255, 255);

    z_up();

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
    render_ctx_free();
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
        .window_flags = mg_config->video.fullscreen ? GS_WINDOW_FLAGS_FULLSCREEN : 0,
        .window_width = mg_config->video.width,
        .window_height = mg_config->video.height,
        .enable_vsync = mg_config->video.vsync,
        .frame_rate = mg_config->video.max_fps,
    };
}