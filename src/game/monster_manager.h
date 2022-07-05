#ifndef MG_MONSTER_MANAGER_H
#define MG_MONSTER_MANAGER_H

#include <gs/gs.h>

#include "../entities/monster.h"

typedef struct mg_monster_manager_t
{
	gs_dyn_array(mg_monster_t *) monsters;
} mg_monster_manager_t;

void mg_monster_manager_init();
void mg_monster_manager_free();
void mg_monster_manager_update();

bool mg_monster_manager_spawn_monster(const gs_vec3 pos, const char *model_path);

extern mg_monster_manager_t *g_monster_manager;

#endif // MG_MONSTER_MANAGER_H