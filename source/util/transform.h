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

static inline gs_vec3 mg_sphere_to_normal(uint8_t *dir)
{
    float32_t phi = dir[0] * 360.0f / 255.0f - 90.0f;
    float32_t theta = dir[1] * 360.0f / 255.0f + 0.0f;

    // clang-format off
    return gs_vec3_norm(
        gs_quat_rotate(
            gs_quat_add(
                gs_quat_angle_axis(gs_deg2rad(phi), MG_AXIS_RIGHT),
                gs_quat_angle_axis(gs_deg2rad(theta), MG_AXIS_UP)
            ),
            MG_AXIS_FORWARD
        )
    );
    // clang-format on
}

static inline gs_vec3 mg_int16_to_vec3(int16_t value)
{
    gs_vec3 vec = gs_default_val();
    float32_t lat = ((value >> 8) & 255) * (2 * GS_PI) / 255;
    float32_t lng = (value & 255) * (2 * GS_PI) / 255;
    vec.x = cos(lat) * sin(lng);
    vec.y = sin(lat) * sin(lng);
    vec.z = cos(lng);
    return vec;
}

#endif // MG_TRANSFORM_H