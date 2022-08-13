/*================================================================
	* game/time_manager.h
	*
	* Copyright (c) 2022 Lauri Räsänen
	* ================================

	-
=================================================================*/

#ifndef MG_TIME_MANAGER_H
#define MG_TIME_MANAGER_H

#include <gs/gs.h>

typedef struct mg_time_manager_t
{
	double update;
	double render;

	double _update_start;
	double _update_end;
	double _render_start;
	double _render_end;
} mg_time_manager_t;

void mg_time_manager_init();
void mg_time_manager_free();
void mg_time_manager_update_start();
void mg_time_manager_update_end();
void mg_time_manager_render_start();
void mg_time_manager_render_end();

extern mg_time_manager_t *g_time_manager;

#endif // MG_TIME_MANAGER_H