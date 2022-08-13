/*================================================================
	* game/time_manager.c
	*
	* Copyright (c) 2022 Lauri Räsänen
	* ================================

	-
=================================================================*/

#include "time_manager.h"

mg_time_manager_t *g_time_manager;

void mg_time_manager_init()
{
	g_time_manager = gs_malloc_init(mg_time_manager_t);
}

void mg_time_manager_free()
{
	gs_free(g_time_manager);
}

void mg_time_manager_update_start()
{
	g_time_manager->_update_start = gs_platform_elapsed_time();
}

void mg_time_manager_update_end()
{
	g_time_manager->_update_end = gs_platform_elapsed_time();
	g_time_manager->update	    = g_time_manager->_update_end - g_time_manager->_update_start;
}

void mg_time_manager_render_start()
{
	g_time_manager->_render_start = gs_platform_elapsed_time();
}

void mg_time_manager_render_end()
{
	g_time_manager->_render_end = gs_platform_elapsed_time();
	g_time_manager->render	    = g_time_manager->_render_end - g_time_manager->_render_start;
}