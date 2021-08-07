/*================================================================
    * util/math.h
    *
    * Copyright (c) 2021 Lauri Räsänen
    * ================================

    ...
=================================================================*/

#ifndef MG_MATH_H
#define MG_MATH_H

#include <gs/gs.h>

#define rand_range(MIN, MAX) \
    (rand() % (MAX - MIN + 1) + MIN)

static inline bool32_t point_in_front_of_plane(gs_vec3 plane_normal, float32_t plane_dist, gs_vec3 point)
{
    float32_t distance = gs_vec3_dot(point, plane_normal) - plane_dist;

    if (distance >= 0.0f)
        return true;

    return false;
}

static inline gs_vec3 mg_clip_velocity(gs_vec3 velocity, gs_vec3 plane_normal, float overbounce)
{
    float backoff = gs_vec3_dot(velocity, plane_normal);

    if (backoff < 0)
    {
        backoff *= overbounce;
    }
    else
    {
        backoff /= overbounce;
    }

    gs_vec3 change = gs_vec3_scale(plane_normal, backoff);
    return gs_vec3_sub(velocity, change);
}

static inline float32_t mg_lerp(float32_t start, float32_t end, float32_t frac)
{
    return start + (end - start) * frac;
}

#endif // MG_MATH_H