#include "game_manager.h"
#include "../game/console.h"
#include "../graphics/renderer.h"

mg_game_manager_t *g_game_manager;

void mg_game_manager_init()
{
	g_game_manager	       = gs_malloc_init(mg_game_manager_t);
	g_game_manager->player = mg_player_new();

	mg_game_manager_load_map("maps/q3dm1.bsp");
	mg_game_manager_spawn_player();

	mg_cmd_arg_type types[] = {MG_CMD_ARG_STRING};
	mg_cmd_new("map", "Load map", &mg_game_manager_load_map, (mg_cmd_arg_type *)types, 1);
	mg_cmd_new("spawn", "Spawn player", &mg_game_manager_spawn_player, NULL, 0);
}

void mg_game_manager_free()
{
	mg_player_free(g_game_manager->player);
	g_game_manager->player = NULL;
	bsp_map_free(g_game_manager->map);
	g_game_manager->map = NULL;

	gs_free(g_game_manager);
	g_game_manager = NULL;
}

void mg_game_manager_update()
{
	mg_player_update(g_game_manager->player);
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