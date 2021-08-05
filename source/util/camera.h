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

#include "transform.h"

static inline gs_mat4 mg_camera_get_view(gs_camera_t *cam)
{
    gs_vec3 up = gs_quat_rotate(cam->transform.rotation, MG_AXIS_UP);
    gs_vec3 forward = gs_quat_rotate(cam->transform.rotation, MG_AXIS_FORWARD);
    gs_vec3 target = gs_vec3_add(forward, cam->transform.position);
    return gs_mat4_look_at(cam->transform.position, target, up);
}

static inline gs_mat4 mg_camera_get_view_projection(gs_camera_t *cam, s32 view_width, s32 view_height)
{
    gs_mat4 view = mg_camera_get_view(cam);
    gs_mat4 proj = gs_camera_get_proj(cam, view_width, view_height);
    return gs_mat4_mul(proj, view);
}

#endif // MG_UTIL_CAMERA_H