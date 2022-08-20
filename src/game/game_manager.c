#include "game_manager.h"
#include "../graphics/renderer.h"
#include "../graphics/ui_manager.h"
#include "../util/transform.h"
#include "config.h"
#include "console.h"
#include "monster_manager.h"
#include "time_manager.h"

mg_game_manager_t *g_game_manager;

void mg_game_manager_init()
{
	g_game_manager	       = gs_malloc_init(mg_game_manager_t);
	g_game_manager->player = mg_player_new();

	mg_game_manager_load_map("assets/maps/q3dm1.bsp");
	mg_game_manager_spawn_player();

	mg_monster_manager_init();

	mg_cmd_arg_type types[] = {MG_CMD_ARG_STRING};
	mg_cmd_new("map", "Load map", &mg_game_manager_load_map, (mg_cmd_arg_type *)types, 1);
	mg_cmd_new("spawn", "Spawn player", &mg_game_manager_spawn_player, NULL, 0);
}

void mg_game_manager_free()
{
	mg_monster_manager_free();

	mg_player_free(g_game_manager->player);
	g_game_manager->player = NULL;
	bsp_map_free(g_game_manager->map);
	g_game_manager->map = NULL;

	gs_free(g_game_manager);
	g_game_manager = NULL;
}

void mg_game_manager_update()
{
	if (g_ui_manager->console_open)
	{
		mg_game_manager_input_console();
	}
	else if (g_ui_manager->menu_open)
	{
		mg_game_manager_input_menu();
	}
	else if (g_game_manager->player)
	{
		mg_game_manager_input_alive();
		mg_player_update(g_game_manager->player);
		mg_monster_manager_update();
	}

	mg_game_manager_input_general();
}

void mg_game_manager_load_map(char *filename)
{
	if (!gs_platform_file_exists(filename))
	{
		mg_println("mg_game_manager_load_map() failed: file not found '%s'", filename);
		return false;
	}

	if (g_game_manager->map != NULL)
	{
		bsp_map_free(g_game_manager->map);
		g_game_manager->map = NULL;
	}

	g_game_manager->map = gs_malloc_init(bsp_map_t);
	load_bsp(filename, g_game_manager->map);

	if (g_game_manager->map->valid)
	{
		bsp_map_init(g_game_manager->map);
		mg_game_manager_spawn_player();
	}
	else
	{
		mg_println("Failed to load map %s", filename);
		bsp_map_free(g_game_manager->map);
		g_game_manager->map = NULL;
	}
}

void mg_game_manager_spawn_player()
{
	if (g_game_manager->map->valid)
	{
		g_game_manager->player->velocity     = gs_v3(0, 0, 0);
		g_game_manager->player->camera.pitch = 0;
		bsp_map_find_spawn_point(g_game_manager->map, &g_game_manager->player->transform.position, &g_game_manager->player->yaw);
		g_game_manager->player->last_valid_pos = g_game_manager->player->transform.position;
		g_game_manager->player->yaw -= 90;
		g_renderer->cam = &g_game_manager->player->camera.cam;
	}
}

#ifdef __ANDROID__
mg_player_input_t mg_game_manager_get_input()
{
	mg_player_input_t input = {0};

	// Testing
	input.move.x += 1.0f;

	if (gs_platform_touch_down(0))
	{
		input.delta_aim = gs_vec2_scale(gs_platform_touch_deltav(0), mg_cvar("cl_sensitivity")->value.f * 0.022f);
	}

	return input;
}
#else
mg_player_input_t mg_game_manager_get_input()
{
	double dt		= g_time_manager->unscaled_delta;
	mg_player_input_t input = {0};

	input.delta_aim = gs_vec2_scale(gs_platform_mouse_deltav(), mg_cvar("cl_sensitivity")->value.f * 0.022f);

	f32 scroll_x, scroll_y;
	gs_platform_mouse_wheel(&scroll_x, &scroll_y);

	if (gs_platform_key_down(GS_KEYCODE_UP))
		input.delta_aim.y -= 150.0f * dt;
	if (gs_platform_key_down(GS_KEYCODE_DOWN))
		input.delta_aim.y += 150.0f * dt;
	if (gs_platform_key_down(GS_KEYCODE_RIGHT))
		input.delta_aim.x += 150.0f * dt;
	if (gs_platform_key_down(GS_KEYCODE_LEFT))
		input.delta_aim.x -= 150.0f * dt;

	if (gs_platform_key_down(GS_KEYCODE_W))
		input.move.x += 1.0f;
	if (gs_platform_key_down(GS_KEYCODE_S))
		input.move.x -= 1.0f;
	if (gs_platform_key_down(GS_KEYCODE_D))
		input.move.y += 1.0f;
	if (gs_platform_key_down(GS_KEYCODE_A))
		input.move.y -= 1.0f;

	if (gs_platform_key_down(GS_KEYCODE_SPACE))
		input.jump = true;
	if (gs_platform_key_down(GS_KEYCODE_LEFT_CONTROL))
		input.crouch = true;

	if (gs_platform_key_down(GS_MOUSE_LBUTTON))
		input.shoot = true;

	input.wish_slot = -1;

	if (gs_platform_key_pressed(GS_KEYCODE_1))
		input.wish_slot = 0;

	if (gs_platform_key_pressed(GS_KEYCODE_2))
		input.wish_slot = 1;

	if (gs_platform_key_pressed(GS_KEYCODE_3))
		input.wish_slot = 2;

	if (gs_platform_key_pressed(GS_KEYCODE_4))
		input.wish_slot = 3;

	if (gs_platform_key_pressed(GS_KEYCODE_5))
		input.wish_slot = 4;

	if (gs_platform_key_pressed(GS_KEYCODE_6))
		input.wish_slot = 5;

	if (gs_platform_key_pressed(GS_KEYCODE_7))
		input.wish_slot = 6;

	if (gs_platform_key_pressed(GS_KEYCODE_8))
		input.wish_slot = 7;

	if (gs_platform_key_pressed(GS_KEYCODE_9))
		input.wish_slot = 8;

	if (gs_platform_key_pressed(GS_KEYCODE_0))
		input.wish_slot = 9;

	// TODO: weapon scroll
	// if (scroll_y > 0)
	// 	...
	// else if (scroll_y < 0)
	// 	...

	return input;
}
#endif

void mg_game_manager_input_alive()
{
	gs_platform_t *platform = gs_subsystem(platform);

	// Reset
	g_game_manager->player->wish_move   = gs_v3(0, 0, 0);
	g_game_manager->player->wish_jump   = false;
	g_game_manager->player->wish_crouch = false;

	mg_player_input_t input = mg_game_manager_get_input();

	// Rotate
	g_game_manager->player->camera.pitch	   = gs_clamp(g_game_manager->player->camera.pitch + input.delta_aim.y, -90.0f, 90.0f);
	g_game_manager->player->yaw		   = fmodf(g_game_manager->player->yaw - input.delta_aim.x, 360.0f);
	g_game_manager->player->transform.rotation = gs_quat_angle_axis(gs_deg2rad(g_game_manager->player->yaw), MG_AXIS_UP);

	// Move dir
	if (input.move.x != 0)
		g_game_manager->player->wish_move = gs_vec3_add(g_game_manager->player->wish_move, gs_vec3_scale(mg_get_forward(g_game_manager->player->transform.rotation), input.move.x));
	if (input.move.y != 0)
		g_game_manager->player->wish_move = gs_vec3_add(g_game_manager->player->wish_move, gs_vec3_scale(mg_get_right(g_game_manager->player->transform.rotation), input.move.y));

	g_game_manager->player->wish_move.z = 0;
	g_game_manager->player->wish_move   = gs_vec3_norm(g_game_manager->player->wish_move);

	// Actions
	g_game_manager->player->wish_jump   = input.jump;
	g_game_manager->player->wish_crouch = input.crouch;
	g_game_manager->player->wish_shoot  = input.shoot;

	if (input.wish_slot >= 0)
	{
		mg_player_switch_weapon(g_game_manager->player, input.wish_slot);
	}

	// TODO: platform
	if (gs_platform_key_pressed(GS_KEYCODE_ESC))
	{
		g_ui_manager->menu_open = true;
	}

	if (gs_platform_key_pressed(GS_KEYCODE_F))
	{
		mg_monster_manager_spawn_monster(
			gs_vec3_add(g_game_manager->player->transform.position, gs_vec3_scale(mg_get_forward(g_game_manager->player->transform.rotation), 250.0f)),
			"cube.md3");
	}
}

void mg_game_manager_input_console()
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

	if (gs_platform_key_pressed(GS_KEYCODE_ESC))
	{
		g_ui_manager->console_open = false;
	}
}

void mg_game_manager_input_menu()
{
	if (gs_platform_key_pressed(GS_KEYCODE_ESC))
	{
		g_ui_manager->menu_open = false;
	}
}

void mg_game_manager_input_general()
{
	if (gs_platform_key_pressed(GS_KEYCODE_F1))
	{
		g_ui_manager->console_open     = !g_ui_manager->console_open;
		g_ui_manager->console_scroll_y = 0;
		g_ui_manager->console_scroll_x = 0;
	}

	if (gs_platform_key_pressed(GS_KEYCODE_F2))
	{
		g_ui_manager->debug_open = !g_ui_manager->debug_open;
	}

	if (gs_platform_key_pressed(GS_KEYCODE_F3))
	{
		mg_cvar_t *fs = mg_cvar("vid_fullscreen");
		fs->value.i   = !fs->value.i;
	}
}
