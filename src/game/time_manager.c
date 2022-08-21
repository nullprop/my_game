/*================================================================
	* game/time_manager.c
	*
	* Copyright (c) 2022 Lauri Räsänen
	* ================================

	-
=================================================================*/

#include "time_manager.h"
#include "config.h"

mg_time_manager_t *g_time_manager;

void mg_time_manager_init()
{
	g_time_manager = gs_malloc_init(mg_time_manager_t);

	// Avoid possible division by 0 elsewhere
	g_time_manager->delta	       = DBL_MIN;
	g_time_manager->unscaled_delta = DBL_MIN;
	g_time_manager->unscaled_time  = DBL_MIN;
	g_time_manager->time	       = DBL_MIN;
}

void mg_time_manager_free()
{
	gs_free(g_time_manager);
}

void mg_time_manager_update_start()
{
	g_time_manager->_update_start  = gs_platform_elapsed_time() / 1000.0f;
	g_time_manager->unscaled_delta = gs_platform_delta_time();
	g_time_manager->delta	       = g_time_manager->unscaled_delta * mg_cvar("cl_timescale")->value.f;
	g_time_manager->unscaled_time += g_time_manager->unscaled_delta;
	g_time_manager->time += g_time_manager->delta;
}

void mg_time_manager_update_end()
{
	g_time_manager->_update_end = gs_platform_elapsed_time() / 1000.0f;
	g_time_manager->update	    = g_time_manager->_update_end - g_time_manager->_update_start;
}

void mg_time_manager_render_start()
{
	g_time_manager->_render_start = gs_platform_elapsed_time() / 1000.0f;
}

void mg_time_manager_render_end()
{
	g_time_manager->_render_end = gs_platform_elapsed_time() / 1000.0f;
	g_time_manager->render	    = g_time_manager->_render_end - g_time_manager->_render_start;
}

void mg_time_manager_bsp_start()
{
	g_time_manager->_bsp_start = gs_platform_elapsed_time() / 1000.0f;
}

void mg_time_manager_bsp_end()
{
	g_time_manager->_bsp_end = gs_platform_elapsed_time() / 1000.0f;
	g_time_manager->bsp	 = g_time_manager->_bsp_end - g_time_manager->_bsp_start;
}

void mg_time_manager_vis_start()
{
	g_time_manager->_vis_start = gs_platform_elapsed_time() / 1000.0f;
}

void mg_time_manager_vis_end()
{
	g_time_manager->_vis_end = gs_platform_elapsed_time() / 1000.0f;
	g_time_manager->vis	 = g_time_manager->_vis_end - g_time_manager->_vis_start;
}

void mg_time_manager_models_start()
{
	g_time_manager->_models_start = gs_platform_elapsed_time() / 1000.0f;
}

void mg_time_manager_models_end()
{
	g_time_manager->_models_end = gs_platform_elapsed_time() / 1000.0f;
	g_time_manager->models	    = g_time_manager->_models_end - g_time_manager->_models_start;
}

void mg_time_manager_viewmodel_start()
{
	g_time_manager->_viewmodel_start = gs_platform_elapsed_time() / 1000.0f;
}

void mg_time_manager_viewmodel_end()
{
	g_time_manager->_viewmodel_end = gs_platform_elapsed_time() / 1000.0f;
	g_time_manager->viewmodel      = g_time_manager->_viewmodel_end - g_time_manager->_viewmodel_start;
}

void mg_time_manager_post_start()
{
	g_time_manager->_post_start = gs_platform_elapsed_time() / 1000.0f;
}

void mg_time_manager_post_end()
{
	g_time_manager->_post_end = gs_platform_elapsed_time() / 1000.0f;
	g_time_manager->post	  = g_time_manager->_post_end - g_time_manager->_post_start;
}

void mg_time_manager_ui_start()
{
	g_time_manager->_ui_start = gs_platform_elapsed_time() / 1000.0f;
}

void mg_time_manager_ui_end()
{
	g_time_manager->_ui_end = gs_platform_elapsed_time() / 1000.0f;
	g_time_manager->ui	= g_time_manager->_ui_end - g_time_manager->_ui_start;
}

void mg_time_manager_submit_start()
{
	g_time_manager->_submit_start = gs_platform_elapsed_time() / 1000.0f;
}

void mg_time_manager_submit_end()
{
	g_time_manager->_submit_end = gs_platform_elapsed_time() / 1000.0f;
	g_time_manager->submit	    = g_time_manager->_submit_end - g_time_manager->_submit_start;
}
