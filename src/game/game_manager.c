#include "game_manager.h"
#include "../graphics/renderer.h"
#include "../graphics/ui_manager.h"
#include "../util/transform.h"
#include "config.h"
#include "console.h"
#include "monster_manager.h"

mg_game_manager_t *g_game_manager;

void mg_game_manager_init()
{
	g_game_manager	       = gs_malloc_init(mg_game_manager_t);
	g_game_manager->player = mg_player_new();

	mg_game_manager_load_map("maps/q3dm1.bsp");
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
		g_renderer->bsp = g_game_manager->map;
		mg_game_manager_spawn_player();
	}
	else
	{
		mg_println("Failed to load map %s", filename);
		bsp_map_free(g_game_manager->map);
		g_game_manager->map = NULL;
		g_renderer->bsp	    = NULL;
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

void mg_game_manager_input_alive()
{
	gs_platform_t *platform = gs_subsystem(platform);
	float dt		= platform->time.delta;
	double pt		= gs_platform_elapsed_time();

	g_game_manager->player->wish_move   = gs_v3(0, 0, 0);
	g_game_manager->player->wish_jump   = false;
	g_game_manager->player->wish_crouch = false;

	gs_vec2 dp = gs_vec2_scale(gs_platform_mouse_deltav(), mg_cvar("cl_sensitivity")->value.f * 0.022f);

	if (gs_platform_key_down(GS_KEYCODE_UP))
		dp.y -= 150.0f * dt;
	if (gs_platform_key_down(GS_KEYCODE_DOWN))
		dp.y += 150.0f * dt;
	if (gs_platform_key_down(GS_KEYCODE_RIGHT))
		dp.x += 150.0f * dt;
	if (gs_platform_key_down(GS_KEYCODE_LEFT))
		dp.x -= 150.0f * dt;

	// Rotate
	g_game_manager->player->camera.pitch	   = gs_clamp(g_game_manager->player->camera.pitch + dp.y, -90.0f, 90.0f);
	g_game_manager->player->yaw		   = fmodf(g_game_manager->player->yaw - dp.x, 360.0f);
	g_game_manager->player->transform.rotation = gs_quat_angle_axis(gs_deg2rad(g_game_manager->player->yaw), MG_AXIS_UP);

	if (gs_platform_key_down(GS_KEYCODE_W))
		g_game_manager->player->wish_move = gs_vec3_add(g_game_manager->player->wish_move, mg_get_forward(g_game_manager->player->transform.rotation));
	if (gs_platform_key_down(GS_KEYCODE_S))
		g_game_manager->player->wish_move = gs_vec3_add(g_game_manager->player->wish_move, mg_get_backward(g_game_manager->player->transform.rotation));
	if (gs_platform_key_down(GS_KEYCODE_D))
		g_game_manager->player->wish_move = gs_vec3_add(g_game_manager->player->wish_move, mg_get_right(g_game_manager->player->transform.rotation));
	if (gs_platform_key_down(GS_KEYCODE_A))
		g_game_manager->player->wish_move = gs_vec3_add(g_game_manager->player->wish_move, mg_get_left(g_game_manager->player->transform.rotation));

	g_game_manager->player->wish_move.z = 0;
	g_game_manager->player->wish_move   = gs_vec3_norm(g_game_manager->player->wish_move);

	if (gs_platform_key_down(GS_KEYCODE_SPACE))
		g_game_manager->player->wish_jump = true;
	if (gs_platform_key_down(GS_KEYCODE_LEFT_CONTROL))
		g_game_manager->player->wish_crouch = true;

	if (gs_platform_key_pressed(GS_KEYCODE_ESC))
	{
		g_ui_manager->menu_open = true;
	}

	if (gs_platform_key_pressed(GS_KEYCODE_F))
	{
		mg_monster_manager_spawn_monster(
			gs_vec3_add(g_game_manager->player->transform.position, gs_vec3_scale(mg_get_forward(g_game_manager->player->transform.rotation), 250.0f)),
			"models/cube.md3");
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
