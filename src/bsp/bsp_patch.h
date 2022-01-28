/*================================================================
	* bsp/bsp_patch.h
	*
	* Copyright (c) 2021 Lauri Räsänen
	* ================================

	...
=================================================================*/

#ifndef BSP_PATCH_H
#define BSP_PATCH_H

#include "bsp_types.h"

// Helpers for vertex lump math
static inline bsp_vert_lump_t bsp_vert_lump_mul(bsp_vert_lump_t lump, float32_t mul)
{
	bsp_vert_lump_t result = {
		.position  = gs_vec3_scale(lump.position, mul),
		.tex_coord = gs_vec2_scale(lump.tex_coord, mul),
		.lm_coord  = gs_vec2_scale(lump.lm_coord, mul),
		.normal	   = lump.normal,
		.color	   = lump.color};
	return result;
}

static inline bsp_vert_lump_t bsp_vert_lump_add(bsp_vert_lump_t a, bsp_vert_lump_t b)
{
	bsp_vert_lump_t result = {
		.position  = gs_vec3_add(a.position, b.position),
		.tex_coord = gs_vec2_add(a.tex_coord, b.tex_coord),
		.lm_coord  = gs_vec2_add(a.lm_coord, b.lm_coord),
		.normal	   = a.normal,
		.color	   = a.color};
	return result;
}

void bsp_quadratic_patch_tesselate(bsp_quadratic_patch_t *patch);
void bsp_quadratic_patch_free(bsp_quadratic_patch_t *patch);
void bsp_patch_free(bsp_patch_t *patch);

#endif // BSP_PATCH_H