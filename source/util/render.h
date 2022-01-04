/*================================================================
	* util/render.h
	*
	* Copyright (c) 2021 Lauri Räsänen
	* ================================

	Rendering utilities.
=================================================================*/

#ifndef MG_RENDER_H
#define MG_RENDER_H

#include <gs/gs.h>

// Generate black and pink grid for a missing texture
static inline gs_color_t *mg_get_missing_texture_pixels(uint32_t size)
{
	gs_color_t pink	   = gs_color(255, 0, 220, 255);
	gs_color_t black   = gs_color(0, 0, 0, 255);
	gs_color_t *pixels = (gs_color_t *)gs_malloc(size * size * sizeof(gs_color_t));
	for (uint32_t row = 0; row < size; row++)
	{
		for (uint32_t col = 0; col < size; col++)
		{
			uint32_t idx = row * size + col;
			if ((row % 2) == (col % 2))
			{
				pixels[idx] = pink;
			}
			else
			{
				pixels[idx] = black;
			}
		}
	}

	return pixels;
}

#endif // MG_RENDER_H