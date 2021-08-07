/*================================================================
    * bsp/bsp_trace.h
    *
    * Copyright (c) 2021 Lauri Räsänen
    * ================================

    Types for BSP tracing.
=================================================================*/

#ifndef BSP_TRACE_H
#define BSP_TRACE_H

#include <gs/gs.h>

#include "bsp_types.h"

typedef enum bsp_trace_type
{
    RAY,
    SPHERE,
    BOX,
} bsp_trace_type;

typedef struct bsp_trace_t
{
    bsp_map_t *map;
    bsp_trace_type type;
    float32_t fraction;
    float32_t radius;
    bool32_t start_solid;
    bool32_t all_solid;
    gs_vec3 end;
    gs_vec3 normal;
    gs_vec3 mins;
    gs_vec3 maxs;
    gs_vec3 extents;
    int32_t contents;
    int32_t surface_flags;
} bsp_trace_t;

void bsp_trace_ray(bsp_trace_t *trace, gs_vec3 start, gs_vec3 end);
void bsp_trace_sphere(bsp_trace_t *trace, gs_vec3 start, gs_vec3 end, float32_t radius);
void bsp_trace_box(bsp_trace_t *trace, gs_vec3 start, gs_vec3 end, gs_vec3 mins, gs_vec3 maxs);
void _bsp_trace(bsp_trace_t *trace, gs_vec3 start, gs_vec3 end);
void _bsp_trace_check_node(bsp_trace_t *trace, int32_t node_index, float32_t start_fraction, float32_t end_fraction, gs_vec3 start, gs_vec3 end);
void _bsp_trace_check_brush(bsp_trace_t *trace, bsp_brush_lump_t brush, gs_vec3 start, gs_vec3 end);

#endif // BSP_TRACE_H