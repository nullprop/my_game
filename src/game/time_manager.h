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
	double update;	       // seconds
	double render;	       // seconds
	double delta;	       // seconds
	double unscaled_delta; // seconds
	double time;	       // seconds
	double unscaled_time;  // seconds

	double _update_start; // seconds
	double _update_end;   // seconds
	double _render_start; // seconds
	double _render_end;   // seconds
} mg_time_manager_t;

void mg_time_manager_init();
void mg_time_manager_free();
void mg_time_manager_update_start();
void mg_time_manager_update_end();
void mg_time_manager_render_start();
void mg_time_manager_render_end();

extern mg_time_manager_t *g_time_manager;

#endif // MG_TIME_MANAGER_H