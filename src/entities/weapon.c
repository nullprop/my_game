/*================================================================
	* entities/weapon.c
	*
	* Copyright (c) 2022 Lauri Räsänen
	* ================================

	...
=================================================================*/

#include "weapon.h"
#include "../game/console.h"
#include "../game/time_manager.h"
#include "../graphics/renderer.h"
#include "../util/transform.h"
#include "rocket.h"

mg_weapon_t *mg_weapon_create(mg_weapon_type type)
{
	mg_weapon_t *weapon = gs_malloc_init(mg_weapon_t);
	weapon->type	    = type;

	// TODO: weapon def files
	switch (type)
	{
	case MG_WEAPON_MACHINE_GUN:
		weapon->model	       = mg_model_manager_find_or_load("weapons/machine_gun.md3", "basic");
		weapon->view_scale     = gs_v3(1.0f, 1.0f, 1.0f);
		weapon->uses_ammo      = true;
		weapon->ammo_type      = MG_AMMO_BULLET;
		weapon->ammo_current   = 50;
		weapon->ammo_max       = 50;
		weapon->shoot_interval = 0.1;
		break;

	case MG_WEAPON_ROCKET_LAUNCHER:
		weapon->model	       = mg_model_manager_find_or_load("weapons/rocket_launcher.md3", "basic");
		weapon->view_scale     = gs_v3(0.6f, 0.6f, 0.6f);
		weapon->uses_ammo      = true;
		weapon->ammo_type      = MG_AMMO_ROCKET;
		weapon->ammo_current   = 10;
		weapon->ammo_max       = 10;
		weapon->shoot_interval = 1.0;
		break;

	default:
		mg_println("mg_weapon_create: unknown weapon type %d", type);
		assert(false);
	}

	if (weapon->model == NULL)
	{
		mg_println("mg_weapon_create: invalid model for type %d", type);
		assert(false);
	}

	weapon->transform     = gs_vqs_default();
	weapon->renderable_id = mg_renderer_create_renderable(*weapon->model, &weapon->transform);

	return weapon;
}

void mg_weapon_free(mg_weapon_t *weapon)
{
	mg_renderer_remove_renderable(weapon->renderable_id);
	gs_free(weapon);
}

mg_weapon_shoot_result mg_weapon_shoot(mg_weapon_t *weapon, gs_vqs origin)
{
	if (weapon->ammo_current <= 0)
	{
		return MG_WEAPON_RESULT_NO_AMMO;
	}

	double time = g_time_manager->time;

	if (time - weapon->last_shot < weapon->shoot_interval)
	{
		return MG_WEAPON_RESULT_COOLDOWN;
	}

	weapon->ammo_current--;
	weapon->last_shot = time;

	switch (weapon->type)
	{
	case MG_WEAPON_MACHINE_GUN:
		// TODO: trace
		break;

	case MG_WEAPON_ROCKET_LAUNCHER:
		mg_rocket_new(gs_vqs_absolute_transform(
			&(gs_vqs){
				.position = gs_vec3_scale(MG_AXIS_DOWN, 8.0f),
				.rotation = gs_quat_default(),
				.scale	  = gs_v3(1.0f, 1.0f, 1.0f),
			},
			&origin));
		break;

	default:
		mg_println("mg_weapon_create: unknown weapon type %d", weapon->type);
		assert(false);
	}

	// TODO: sound

	return MG_WEAPON_RESULT_SHOT;
}