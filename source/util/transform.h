/*================================================================
    * util/transform.h
    *
    * Copyright (c) 2021 Lauri Räsänen
    * ================================

    ...
=================================================================*/

#ifndef MG_TRANSFORM_H
#define MG_TRANSFORM_H

#include <gs/gs.h>

#define MG_AXIS_UP gs_v3(0, 0, 1.0f)
#define MG_AXIS_DOWN gs_v3(0, 0, -1.0f)
#define MG_AXIS_FORWARD gs_v3(0, 1.0f, 0)
#define MG_AXIS_BACKWARD gs_v3(0, -1.0f, 0)
#define MG_AXIS_RIGHT gs_v3(1.0f, 0, 0)
#define MG_AXIS_LEFT gs_v3(-1.0f, 0, 0)

static inline gs_vec3 mg_get_forward(gs_quat rotation)
{
    return (gs_quat_rotate(rotation, MG_AXIS_FORWARD));
}

static inline gs_vec3 mg_get_backward(gs_quat rotation)
{
    return (gs_quat_rotate(rotation, MG_AXIS_BACKWARD));
}

static inline gs_vec3 mg_get_up(gs_quat rotation)
{
    return (gs_quat_rotate(rotation, MG_AXIS_UP));
}

static inline gs_vec3 mg_get_down(gs_quat rotation)
{
    return (gs_quat_rotate(rotation, MG_AXIS_DOWN));
}

static inline gs_vec3 mg_get_right(gs_quat rotation)
{
    return (gs_quat_rotate(rotation, MG_AXIS_RIGHT));
}

static inline gs_vec3 mg_get_left(gs_quat rotation)
{
    return (gs_quat_rotate(rotation, MG_AXIS_LEFT));
}

#endif // MG_TRANSFORM_H