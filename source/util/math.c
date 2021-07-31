/*================================================================
    * util/math.c
    *
    * Copyright (c) 2021 Lauri Räsänen
    * ================================

    ...
=================================================================*/

#include <gs/gs.h>

#define rand_range(MIN, MAX) \
    (rand() % (MAX - MIN + 1) + MIN)

bool32_t point_in_front_of_plane(gs_vec3 plane_normal, float32_t plane_dist, gs_vec3 point)
{
    float32_t distance = gs_vec3_dot(point, plane_normal) - plane_dist;

    if (distance >= 0.0f)
        return true;

    return false;
}