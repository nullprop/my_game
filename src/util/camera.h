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

#ifdef WIN32
// stupid...
#undef near
#undef far
#endif

typedef struct mg_camera_frustum_t
{
	union
	{
		gs_vec4 planes[6];
		struct
		{
			gs_vec4 left;
			gs_vec4 right;
			gs_vec4 bottom;
			gs_vec4 top;
			gs_vec4 near;
			gs_vec4 far;
		};
	};

} mg_camera_frustum_t;

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

static inline mg_camera_frustum_t mg_camera_get_frustum_planes(const gs_mat4 mvp, bool normalize)
{
	mg_camera_frustum_t fr = {0};

	for (size_t i = 0; i < 4; i++)
	{
		fr.left.xyzw[i]	  = mvp.m[i][3] + mvp.m[i][0];
		fr.right.xyzw[i]  = mvp.m[i][3] - mvp.m[i][0];
		fr.bottom.xyzw[i] = mvp.m[i][3] + mvp.m[i][1];
		fr.top.xyzw[i]	  = mvp.m[i][3] - mvp.m[i][1];
		fr.near.xyzw[i]	  = mvp.m[i][3] + mvp.m[i][2];
		fr.far.xyzw[i]	  = mvp.m[i][3] - mvp.m[i][2];
	}

	if (normalize)
	{
		for (size_t i = 0; i < 6; i++)
		{
			// invSqrt(dot(plane))
			float length = 1.0 / sqrt(fr.planes[i].x * fr.planes[i].x + fr.planes[i].y * fr.planes[i].y + fr.planes[i].z * fr.planes[i].z);
			fr.planes[i].x *= length;
		}
	}

	return fr;
}

static inline bool mg_camera_point_in_frustum(const mg_camera_frustum_t fr, const gs_vec3 pos, const float radius)
{
	float dist = 0;
	for (size_t i = 0; i < 6; i++)
	{
		// dot(pos, plane) + plane.w < -radius
		if (((pos.x * fr.planes[i].x) + (pos.y * fr.planes[i].y) + pos.z * fr.planes[i].z) + fr.planes[i].w + radius < 0) return false;
	}
	return true;
}

static inline bool mg_camera_aabb_in_frustum(const mg_camera_frustum_t fr, const gs_vec3 mins, const gs_vec3 maxs)
{
	gs_vec3 center	= gs_vec3_scale(gs_vec3_add(mins, maxs), 0.5);
	float half_size = gs_vec3_len(gs_vec3_sub(maxs, center));

	for (size_t i = 0; i < 6; i++)
	{
		// Distance of AABB center from plane.
		float distance = (fr.planes[i].x * center.x + fr.planes[i].y * center.y + fr.planes[i].z * center.z) + fr.planes[i].w;

		// If center of AABB is more than half-diagonal distance away
		// from any of the 6 planes, it cannot be inside the frustum.
		if (distance < -half_size)
		{
			return false;
		}
		// Intersecting this plane if between +- half-diagonal distance.
		// (Might actually be just outside the frustum and still return
		// true since we are using diagonal distance)
		else if (abs(distance) < half_size)
		{
			return true;
		}
	}

	// No planes intersect, is fully inside the frustum.
	return true;
}

#endif // MG_UTIL_CAMERA_H