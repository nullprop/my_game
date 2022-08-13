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
	double delta;	       // seconds
	double unscaled_delta; // seconds
	double time;	       // seconds
	double unscaled_time;  // seconds

	double update; // seconds
	double render; // seconds
	double bsp;    // seconds
	double vis;    // seconds
	double models; // seconds
	double post;   // seconds
	double ui;     // seconds

	double _update_start; // seconds
	double _update_end;   // seconds
	double _render_start; // seconds
	double _render_end;   // seconds
	double _bsp_start;    // seconds
	double _bsp_end;      // seconds
	double _vis_start;    // seconds
	double _vis_end;      // seconds
	double _models_start; // seconds
	double _models_end;   // seconds
	double _post_start;   // seconds
	double _post_end;     // seconds
	double _ui_start;     // seconds
	double _ui_end;	      // seconds
} mg_time_manager_t;

void mg_time_manager_init();
void mg_time_manager_free();
// TODO: macro these
void mg_time_manager_update_start();
void mg_time_manager_update_end();
void mg_time_manager_render_start();
void mg_time_manager_render_end();
void mg_time_manager_bsp_start();
void mg_time_manager_bsp_end();
void mg_time_manager_vis_start();
void mg_time_manager_vis_end();
void mg_time_manager_models_start();
void mg_time_manager_models_end();
void mg_time_manager_post_start();
void mg_time_manager_post_end();
void mg_time_manager_ui_start();
void mg_time_manager_ui_end();

extern mg_time_manager_t *g_time_manager;

#endif // MG_TIME_MANAGER_H