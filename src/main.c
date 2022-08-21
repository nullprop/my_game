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
#define GS_GUI_IMPL
#include <gs/util/gs_gui.h>

#include "audio/audio_manager.h"
#include "bsp/bsp_loader.h"
#include "bsp/bsp_map.h"
#include "entities/player.h"
#include "game/config.h"
#include "game/console.h"
#include "game/game_manager.h"
#include "game/time_manager.h"
#include "graphics/model_manager.h"
#include "graphics/renderer.h"
#include "graphics/texture_manager.h"
#include "graphics/ui_manager.h"

void app_init()
{
	// Init managers, free in app_shutdown if adding here
	mg_config_init();
	mg_time_manager_init();
	mg_audio_manager_init();
	mg_texture_manager_init();
	mg_model_manager_init();
	mg_renderer_init(gs_platform_main_window());
	mg_ui_manager_init();
	mg_game_manager_init();

	// Lock mouse at start by default
	// gs_platform_lock_mouse(gs_platform_main_window(), true);

#ifndef __ANDROID__
	if (glfwRawMouseMotionSupported())
	{
		glfwSetInputMode(gs_platform_raw_window_handle(gs_platform_main_window()), GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);
	}
#endif

	// - - - -
	// MD3 testing
	mg_model_t *testmodel	      = mg_model_manager_find("players/sarge/head.md3");
	gs_vqs *testmodel_transform   = gs_malloc_init(gs_vqs);
	testmodel_transform->position = gs_v3(660.0f, 778.0f, -10.0f);
	testmodel_transform->rotation = gs_quat_from_euler(0.0f, 0.0f, 0.0f);
	testmodel_transform->scale    = gs_v3(1.0f, 1.0f, 1.0f);
	mg_renderer_create_renderable(*testmodel, testmodel_transform);

	mg_model_t *testmodel_1		= mg_model_manager_find("players/sarge/upper.md3");
	gs_vqs *testmodel_transform_1	= gs_malloc_init(gs_vqs);
	testmodel_transform_1->position = gs_v3(660.0f, 748.0f, -10.0f);
	testmodel_transform_1->rotation = gs_quat_from_euler(0.0f, 0.0f, 0.0f);
	testmodel_transform_1->scale	= gs_v3(1.0f, 1.0f, 1.0f);
	uint32_t id_1			= mg_renderer_create_renderable(*testmodel_1, testmodel_transform_1);

	mg_model_t *testmodel_2		= mg_model_manager_find("players/sarge/lower.md3");
	gs_vqs *testmodel_transform_2	= gs_malloc_init(gs_vqs);
	testmodel_transform_2->position = gs_v3(660.0f, 718.0f, -10.0f);
	testmodel_transform_2->rotation = gs_quat_from_euler(0.0f, 0.0f, 0.0f);
	testmodel_transform_2->scale	= gs_v3(1.0f, 1.0f, 1.0f);
	uint32_t id_2			= mg_renderer_create_renderable(*testmodel_2, testmodel_transform_2);

	mg_model_t *testmodel_3		= mg_model_manager_find_or_load("weapons/rocket_launcher.md3", "basic");
	gs_vqs *testmodel_transform_3	= gs_malloc_init(gs_vqs);
	testmodel_transform_3->position = gs_v3(660.0f, 680.0f, -10.0f);
	testmodel_transform_3->rotation = gs_quat_from_euler(0.0f, 0.0f, 0.0f);
	testmodel_transform_3->scale	= gs_v3(1.0f, 1.0f, 1.0f);
	uint32_t id_3			= mg_renderer_create_renderable(*testmodel_3, testmodel_transform_3);

	mg_model_t *testmodel_4		= mg_model_manager_find_or_load("weapons/machine_gun.md3", "basic");
	gs_vqs *testmodel_transform_4	= gs_malloc_init(gs_vqs);
	testmodel_transform_4->position = gs_v3(660.0f, 600.0f, -10.0f);
	testmodel_transform_4->rotation = gs_quat_from_euler(0.0f, 0.0f, 0.0f);
	testmodel_transform_4->scale	= gs_v3(1.0f, 1.0f, 1.0f);
	uint32_t id_4			= mg_renderer_create_renderable(*testmodel_4, testmodel_transform_4);

	mg_renderer_play_animation(id_1, "TORSO_GESTURE");
	mg_renderer_play_animation(id_2, "LEGS_WALK");
	// - - - -

	// UI test
	mg_ui_manager_set_dialogue(
		"Lorem ipsum dolor sit amet, consectetur adipiscing elit, \
	sed do eiusmod tempor incididunt ut labore et dolore magna aliqua.",
		-1);

#ifdef __ANDROID__
	g_ui_manager->debug_open = true;
#endif
}

void app_update()
{
	mg_time_manager_update_start();
	uint32_t main_window = gs_platform_main_window();

#ifndef __ANDROID__
	gs_vec2 window_size	  = gs_platform_window_sizev(main_window);
	bool32_t is_fullscreen	  = gs_platform_window_fullscreen(main_window);
	mg_cvar_t *vid_width	  = mg_cvar("vid_width");
	mg_cvar_t *vid_height	  = mg_cvar("vid_height");
	mg_cvar_t *vid_fullscreen = mg_cvar("vid_fullscreen");
	if (window_size.x != vid_width->value.i || window_size.y != vid_height->value.i)
	{
		gs_platform_set_window_size(main_window, vid_width->value.i, vid_height->value.i);
	}
	if (is_fullscreen != vid_fullscreen->value.i)
	{
		gs_platform_set_window_fullscreen(main_window, vid_fullscreen->value.i);
	}
#endif

	gs_platform_t *platform = gs_subsystem(platform);

	mg_cvar_t *vid_max_fps = mg_cvar("vid_max_fps");
	if (platform->time.max_fps != vid_max_fps->value.i)
	{
		gs_platform_set_frame_rate(vid_max_fps->value.i);
	}

	mg_cvar_t *r_filter	= mg_cvar("r_filter");
	mg_cvar_t *r_filter_mip = mg_cvar("r_filter_mip");
	mg_cvar_t *r_mips	= mg_cvar("r_mips");
	if (
		g_texture_manager->tex_filter != r_filter->value.i + 1 ||
		g_texture_manager->mip_filter != r_filter_mip->value.i + 1 ||
		g_texture_manager->num_mips != r_mips->value.i)
	{
		mg_texture_manager_set_filter(
			r_filter->value.i + 1,
			r_filter_mip->value.i + 1,
			r_mips->value.i);
	}

#ifndef __ANDROID__
	// If click, then lock again (in case lost)
	if (gs_platform_mouse_pressed(GS_MOUSE_LBUTTON) && !gs_platform_mouse_locked() && !g_ui_manager->show_cursor)
	{
		gs_platform_lock_mouse(main_window, true);
		if (glfwRawMouseMotionSupported())
		{
			glfwSetInputMode(gs_platform_raw_window_handle(main_window), GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);
		}
	}
#endif

	mg_game_manager_update();
	mg_time_manager_update_end();

	mg_renderer_update();
}

void app_shutdown()
{
	mg_game_manager_free();
	mg_renderer_free();
	mg_ui_manager_free();
	mg_model_manager_free();
	mg_texture_manager_free();
	mg_audio_manager_free();
	mg_time_manager_free();
	mg_config_free();
	mg_console_free();
}

gs_app_desc_t gs_main(int32_t argc, char **argv)
{
	mg_console_init();

	return (gs_app_desc_t){
		.init	       = app_init,
		.update	       = app_update,
		.shutdown      = app_shutdown,
		.window_title  = "Game",
		.window_flags  = 0,
		.window_width  = 800,
		.window_height = 600,
		.enable_vsync  = 0,
		.frame_rate    = 60,
	};
}

// "call DEBUG_fc()" in gdb when hitting breakpoint to free cursor on Linux
void DEBUG_fc()
{
	gs_platform_lock_mouse(gs_platform_main_window(), false);
}