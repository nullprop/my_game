/*================================================================
	* util/camera.h
	*
	* Copyright (c) 2021 Lauri Räsänen
	* ================================

	Camera helpers for our coordinate system.
=================================================================*/

#ifndef MG_UTIL_CAMERA_H
#define MG_UTIL_CAMERA_H

#include <gs/gs.h>
#include <gs/util/gs_idraw.h>

#include "transform.h"

static inline gs_mat4 mg_camera_get_view(gs_camera_t *cam)
{
	gs_vec3 up	= mg_get_up(cam->transform.rotation);
	gs_vec3 forward = mg_get_forward(cam->transform.rotation);
	gs_vec3 target	= gs_vec3_add(forward, cam->transform.position);
	return gs_mat4_look_at(cam->transform.position, target, up);
}

static inline gs_mat4 mg_camera_get_view_projection(gs_camera_t *cam, s32 view_width, s32 view_height)
{
	gs_mat4 view = mg_camera_get_view(cam);
	gs_mat4 proj = gs_camera_get_proj(cam, view_width, view_height);
	return gs_mat4_mul(proj, view);
}

static inline void mg_gsi_camera(gs_immediate_draw_t *gsi, gs_camera_t *cam)
{
	// Just grab main window for now. Will need to grab top of viewport stack in future
	gs_vec2 ws = gs_platform_window_sizev(gs_platform_main_window());
	gsi_load_matrix(gsi, mg_camera_get_view_projection(cam, (s32)ws.x, (s32)ws.y));
}

#endif // MG_UTIL_CAMERA_H