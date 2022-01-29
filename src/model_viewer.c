/*================================================================
	* model_viewer.c
	*
	* Copyright (c) 2021 Lauri Räsänen
	* ================================

	The main entry point of my_game model viewer.
=================================================================*/

#define GS_IMPL
#include <gs/gs.h>
#define GS_IMMEDIATE_DRAW_IMPL
#include <gs/util/gs_idraw.h>
#define GS_GUI_IMPL
#include <gs/util/gs_gui.h>

#include "bsp/bsp_loader.h"
#include "bsp/bsp_map.h"
#include "entities/player.h"
#include "graphics/model_manager.h"
#include "graphics/renderer.h"
#include "graphics/texture_manager.h"
#include "graphics/ui_manager.h"
#include "util/transform.h"

gs_camera_t *camera	= NULL;
float32_t camera_yaw	= 0;
float32_t camera_pitch	= 0;
gs_vqs *model_transform = NULL;
char *model_path	= NULL;

void app_spawn()
{
}

void on_window_resize(GLFWwindow *window, int width, int height)
{
	if (width == 0 || height == 0)
	{
		camera->aspect_ratio = 1;
	}
	else
	{
		camera->aspect_ratio = width / height;
	}
}

void app_init()
{
	camera			   = gs_malloc_init(gs_camera_t);
	*camera			   = gs_camera_perspective();
	camera->fov		   = 75.0f;
	camera->aspect_ratio	   = 4.0f / 3.0f;
	camera->near_plane	   = 0.1f;
	camera->far_plane	   = 1000.0f;
	camera->transform.position = gs_v3(0, -50.0f, 0);

	glfwSetWindowSizeCallback(gs_platform_raw_window_handle(gs_platform_main_window()), &on_window_resize);

	// Init managers, free in app_shutdown if adding here
	mg_texture_manager_init();
	mg_model_manager_init();
	mg_renderer_init(gs_platform_main_window());
	mg_ui_manager_init();

	g_renderer->cam = camera;

	// Lock mouse at start by default
	gs_platform_lock_mouse(gs_platform_main_window(), true);
	if (glfwRawMouseMotionSupported())
	{
		glfwSetInputMode(gs_platform_raw_window_handle(gs_platform_main_window()), GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);
	}

	mg_model_t *model = mg_model_manager_find(model_path);
	if (model == NULL)
	{
		_mg_model_manager_load(model_path, "basic");
		model = mg_model_manager_find(model_path);
	}
	model_transform		  = gs_malloc_init(gs_vqs);
	model_transform->position = gs_v3(0, 0, 0);
	model_transform->rotation = gs_quat_from_euler(0.0f, 0.0f, 0.0f);
	model_transform->scale	  = gs_v3(1.0f, 1.0f, 1.0f);
	uint32_t model_id	  = mg_renderer_create_renderable(*model, model_transform);

	mg_renderer_play_animation(model_id, "TORSO_GESTURE");

	g_ui_manager->debug_open = true;

	app_spawn();
}

void app_update()
{
	gs_platform_t *platform = gs_engine_subsystem(platform);
	float delta_time	= platform->time.delta;

	// If click, then lock again (in case lost)
	if (gs_platform_mouse_pressed(GS_MOUSE_LBUTTON) && !gs_platform_mouse_locked() && !g_ui_manager->show_cursor)
	{
		gs_platform_lock_mouse(gs_platform_main_window(), true);
		if (glfwRawMouseMotionSupported())
		{
			glfwSetInputMode(gs_platform_raw_window_handle(gs_platform_main_window()), GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);
		}
	}

	if (gs_platform_key_down(GS_KEYCODE_ESC))
	{
		gs_engine_quit();
	}

	gs_vec3 wish_move = gs_v3(0, 0, 0);
	gs_vec2 dp	  = gs_vec2_scale(gs_platform_mouse_deltav(), 2.0f * 0.022f);

	if (gs_platform_key_down(GS_KEYCODE_UP))
		dp.y -= 150.0f * delta_time;
	if (gs_platform_key_down(GS_KEYCODE_DOWN))
		dp.y += 150.0f * delta_time;
	if (gs_platform_key_down(GS_KEYCODE_RIGHT))
		dp.x += 150.0f * delta_time;
	if (gs_platform_key_down(GS_KEYCODE_LEFT))
		dp.x -= 150.0f * delta_time;

	camera_pitch		   = gs_clamp(camera_pitch - dp.y, -90.0f, 90.0f);
	camera_yaw		   = fmodf(camera_yaw - dp.x, 360.0f);
	camera->transform.rotation = gs_quat_mul(
		gs_quat_angle_axis(gs_deg2rad(camera_yaw), MG_AXIS_UP),
		gs_quat_angle_axis(gs_deg2rad(camera_pitch), MG_AXIS_RIGHT));

	if (gs_platform_key_down(GS_KEYCODE_W))
		wish_move = gs_vec3_add(wish_move, mg_get_forward(camera->transform.rotation));
	if (gs_platform_key_down(GS_KEYCODE_S))
		wish_move = gs_vec3_add(wish_move, mg_get_backward(camera->transform.rotation));
	if (gs_platform_key_down(GS_KEYCODE_D))
		wish_move = gs_vec3_add(wish_move, mg_get_right(camera->transform.rotation));
	if (gs_platform_key_down(GS_KEYCODE_A))
		wish_move = gs_vec3_add(wish_move, mg_get_left(camera->transform.rotation));
	if (gs_platform_key_down(GS_KEYCODE_SPACE))
		wish_move = gs_vec3_add(wish_move, MG_AXIS_UP);
	if (gs_platform_key_down(GS_KEYCODE_LEFT_CONTROL))
		wish_move = gs_vec3_add(wish_move, MG_AXIS_DOWN);

	wish_move		   = gs_vec3_norm(wish_move);
	camera->transform.position = gs_vec3_add(camera->transform.position, gs_vec3_scale(wish_move, 200.0f * delta_time));

	if (gs_platform_key_pressed(GS_KEYCODE_F2)) g_ui_manager->console_open = !g_ui_manager->console_open;
	if (gs_platform_key_pressed(GS_KEYCODE_F3)) g_ui_manager->debug_open = !g_ui_manager->debug_open;

	mg_renderer_update();
}

void app_shutdown()
{
	mg_renderer_free();
	mg_ui_manager_free();
	mg_model_manager_free();
	mg_texture_manager_free();
	gs_free(camera);
	gs_free(model_transform);
	gs_free(model_path);
}

gs_app_desc_t gs_main(int32_t argc, char **argv)
{
	if (argc < 2)
	{
		gs_println("Missing model path argument");
		gs_engine_quit();
		return;
	}

	size_t sz  = gs_string_length(argv[1]) + 1;
	model_path = gs_malloc(sz);
	memcpy(model_path, argv[1], sz);

	return (gs_app_desc_t){
		.init	       = app_init,
		.update	       = app_update,
		.shutdown      = app_shutdown,
		.window_flags  = 0,
		.window_width  = 800,
		.window_height = 600,
		.enable_vsync  = 1,
		.frame_rate    = 60,
	};
}