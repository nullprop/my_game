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
#include "graphics/model_manager.h"
#include "graphics/renderer.h"
#include "graphics/texture_manager.h"
#include "graphics/ui_manager.h"

void app_init()
{
	// Init managers, free in app_shutdown if adding here
	mg_audio_manager_init();
	mg_texture_manager_init();
	mg_model_manager_init();
	mg_renderer_init(gs_platform_main_window());
	mg_ui_manager_init();
	mg_game_manager_init();

	// Lock mouse at start by default
	// gs_platform_lock_mouse(gs_platform_main_window(), true);
	if (glfwRawMouseMotionSupported())
	{
		glfwSetInputMode(gs_platform_raw_window_handle(gs_platform_main_window()), GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);
	}

	// - - - -
	// MD3 testing
	mg_model_t *testmodel	      = mg_model_manager_find("models/players/sarge/head.md3");
	gs_vqs *testmodel_transform   = gs_malloc_init(gs_vqs);
	testmodel_transform->position = gs_v3(660.0f, 778.0f, -10.0f);
	testmodel_transform->rotation = gs_quat_from_euler(0.0f, 0.0f, 0.0f);
	testmodel_transform->scale    = gs_v3(1.0f, 1.0f, 1.0f);
	mg_renderer_create_renderable(*testmodel, testmodel_transform);

	mg_model_t *testmodel_1		= mg_model_manager_find("models/players/sarge/upper.md3");
	gs_vqs *testmodel_transform_1	= gs_malloc_init(gs_vqs);
	testmodel_transform_1->position = gs_v3(660.0f, 748.0f, -10.0f);
	testmodel_transform_1->rotation = gs_quat_from_euler(0.0f, 0.0f, 0.0f);
	testmodel_transform_1->scale	= gs_v3(1.0f, 1.0f, 1.0f);
	uint32_t id_1			= mg_renderer_create_renderable(*testmodel_1, testmodel_transform_1);

	mg_model_t *testmodel_2		= mg_model_manager_find("models/players/sarge/lower.md3");
	gs_vqs *testmodel_transform_2	= gs_malloc_init(gs_vqs);
	testmodel_transform_2->position = gs_v3(660.0f, 718.0f, -10.0f);
	testmodel_transform_2->rotation = gs_quat_from_euler(0.0f, 0.0f, 0.0f);
	testmodel_transform_2->scale	= gs_v3(1.0f, 1.0f, 1.0f);
	uint32_t id_2			= mg_renderer_create_renderable(*testmodel_2, testmodel_transform_2);

	mg_renderer_play_animation(id_1, "TORSO_GESTURE");
	mg_renderer_play_animation(id_2, "LEGS_WALK");
	// - - - -

	// UI test
	mg_ui_manager_set_dialogue(
		"Lorem ipsum dolor sit amet, consectetur adipiscing elit, \
	sed do eiusmod tempor incididunt ut labore et dolore magna aliqua.",
		-1);
}

void app_update()
{
	// TODO gs_platform_monitor_sizev after updating gs
	GLFWvidmode *vid_mode = glfwGetVideoMode(glfwGetPrimaryMonitor());

	uint32_t main_window	  = gs_platform_main_window();
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

	gs_platform_t *platform = gs_subsystem(platform);

	mg_cvar_t *vid_max_fps = mg_cvar("vid_max_fps");
	if (platform->time.max_fps != vid_max_fps->value.i)
	{
		platform->time.max_fps = vid_max_fps->value.i;
	}

	// TODO: move inputs to separate file
	if (!g_ui_manager->console_open)
	{
		if (gs_platform_key_pressed(GS_KEYCODE_B))
		{
			mg_cvar_t *cvar = mg_cvar("r_barrel_enabled");
			cvar->value.i	= !cvar->value.i;
			mg_println("barrel_enabled: %d", cvar->value.i);
		}
		if (gs_platform_key_pressed(GS_KEYCODE_T))
		{
			mg_cvar_t *cvar = mg_cvar("r_barrel_strength");
			cvar->value.f += 0.1;
			mg_println("barrel_strength: %f", cvar->value.f);
		}
		if (gs_platform_key_pressed(GS_KEYCODE_G))
		{
			mg_cvar_t *cvar = mg_cvar("r_barrel_strength");
			cvar->value.f -= 0.1;
			mg_println("barrel_strength: %f", cvar->value.f);
		}
		if (gs_platform_key_pressed(GS_KEYCODE_Y))
		{
			mg_cvar_t *cvar = mg_cvar("r_barrel_cyl_ratio");
			cvar->value.f += 0.1;
			mg_println("barrel_cyl_ratio: %f", cvar->value.f);
		}
		if (gs_platform_key_pressed(GS_KEYCODE_H))
		{
			mg_cvar_t *cvar = mg_cvar("r_barrel_cyl_ratio");
			cvar->value.f -= 0.1;
			mg_println("barrel_cyl_ratio: %f", cvar->value.f);
		}
		if (gs_platform_key_pressed(GS_KEYCODE_U))
		{
			mg_cvar_t *cvar = mg_cvar("r_fov");
			cvar->value.i += 5;
			mg_println("fov: %zu", cvar->value.i);
		}
		if (gs_platform_key_pressed(GS_KEYCODE_J))
		{
			mg_cvar_t *cvar = mg_cvar("r_fov");
			cvar->value.i -= 5;
			mg_println("fov: %zu", cvar->value.i);
		}

		if (gs_platform_key_pressed(GS_KEYCODE_R))
			mg_game_manager_spawn_player();
	}

	if (gs_platform_key_pressed(GS_KEYCODE_ESC)) g_ui_manager->menu_open = !g_ui_manager->menu_open;
	if (gs_platform_key_pressed(GS_KEYCODE_F1))
	{
		g_ui_manager->console_open     = !g_ui_manager->console_open;
		g_ui_manager->console_scroll_y = 0;
		g_ui_manager->console_scroll_x = 0;
	}
	if (gs_platform_key_pressed(GS_KEYCODE_F2)) g_ui_manager->debug_open = !g_ui_manager->debug_open;
	if (gs_platform_key_pressed(GS_KEYCODE_F3))
	{
		mg_cvar_t *fs = mg_cvar("vid_fullscreen");
		fs->value.i   = !fs->value.i;
	}

	if (g_ui_manager->console_open)
	{
		f32 scroll_x, scroll_y;
		gs_platform_mouse_wheel(&scroll_x, &scroll_y);

		if (gs_platform_key_down(GS_KEYCODE_LSHIFT))
		{
			scroll_x = scroll_y;
			scroll_y = 0;
		}

		if (scroll_y != 0)
		{
			g_ui_manager->console_scroll_y += scroll_y < 0 ? -4 : 4;
			g_ui_manager->console_scroll_y = gs_clamp(g_ui_manager->console_scroll_y, 0, MG_CON_LINES - 1);
		}
		if (scroll_x != 0)
		{
			g_ui_manager->console_scroll_x += scroll_x < 0 ? -4 : 4;
			g_ui_manager->console_scroll_x = gs_min(g_ui_manager->console_scroll_x, 0);
		}

		if (gs_platform_key_pressed(GS_KEYCODE_ENTER))
		{
			mg_console_input(g_ui_manager->console_input);
			memset(g_ui_manager->console_input, 0, 256);
		}
	}

	// If click, then lock again (in case lost)
	if (gs_platform_mouse_pressed(GS_MOUSE_LBUTTON) && !gs_platform_mouse_locked() && !g_ui_manager->show_cursor)
	{
		gs_platform_lock_mouse(gs_platform_main_window(), true);
		if (glfwRawMouseMotionSupported())
		{
			glfwSetInputMode(gs_platform_raw_window_handle(gs_platform_main_window()), GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);
		}
	}

	mg_game_manager_update();
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

	mg_config_free();
	mg_console_free();
}

gs_app_desc_t gs_main(int32_t argc, char **argv)
{
	mg_console_init();
	mg_config_init();

	return (gs_app_desc_t){
		.init	       = app_init,
		.update	       = app_update,
		.shutdown      = app_shutdown,
		.window_title  = "Game",
		.window_flags  = mg_cvar("vid_fullscreen")->value.i ? GS_WINDOW_FLAGS_FULLSCREEN : 0,
		.window_width  = mg_cvar("vid_width")->value.i,
		.window_height = mg_cvar("vid_height")->value.i,
		.enable_vsync  = mg_cvar("vid_vsync")->value.i,
		.frame_rate    = mg_cvar("vid_max_fps")->value.i,
	};
}