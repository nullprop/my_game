#include "monster_manager.h"
#include "../entities/monster.h"
#include "../graphics/renderer.h"
#include "../graphics/ui_manager.h"
#include "../util/transform.h"
#include "config.h"
#include "console.h"

mg_monster_manager_t *g_monster_manager;

void mg_monster_manager_init()
{
	g_monster_manager	    = gs_malloc_init(mg_monster_manager_t);
	g_monster_manager->monsters = gs_dyn_array_new(mg_monster_t *);

	// TODO
	// mg_cmd_arg_type types[] = {MG_CMD_ARG_STRING};
	// mg_cmd_new("monster", "Spawn monster", &mg_monster_manager_spawn_monster, (mg_cmd_arg_type *)types, 1);
}

void mg_monster_manager_free()
{
	for (size_t i = 0; i < gs_dyn_array_size(g_monster_manager->monsters); i++)
	{
		mg_monster_free(g_monster_manager->monsters[i]);
		g_monster_manager->monsters[i] = NULL;
	}

	gs_dyn_array_free(g_monster_manager->monsters);
	gs_free(g_monster_manager);
	g_monster_manager = NULL;
}

void mg_monster_manager_update()
{
	for (size_t i = 0; i < gs_dyn_array_size(g_monster_manager->monsters); i++)
	{
		mg_monster_update(g_monster_manager->monsters[i]);
	}
}

bool mg_monster_manager_spawn_monster(const gs_vec3 pos, const char *model_path)
{
	mg_monster_t *mon = mg_monster_new(model_path, gs_v3(-16.0f, -16.0f, 0), gs_v3(16.0f, 16.0f, 64.0f));
	if (mon)
	{
		mon->transform.position = pos;
		mon->last_valid_pos	= pos;
		gs_dyn_array_push(g_monster_manager->monsters, mon);
		return true;
	}
	return false;
}
