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

static inline void mg_text_to_lines(const gs_asset_font_t *font, const char *text, const uint32_t width, char **lines, uint32_t *num_lines)
{
	gs_vec2 space		= gs_asset_font_text_dimensions(font, " ", -1);
	float32_t current_width = 0;

	size_t sz      = gs_string_length(text) + 1;
	size_t line_sz = sz;
	char tmp[sz];
	strcpy(tmp, text);

	lines[0] = gs_malloc(sz);
	memset(lines[0], '\0', sz);

	char *token = strtok(tmp, " ");
	while (token)
	{
		gs_vec2 v = gs_asset_font_text_dimensions(font, token, -1);
		current_width += v.x + space.x;
		if (current_width > width)
		{
			current_width = 0;
			(*num_lines)++;
			line_sz++;
			lines[*num_lines] = gs_malloc(line_sz);
			memset(lines[*num_lines], '\0', line_sz);
		}

		strcat(lines[*num_lines], token);
		strcat(lines[*num_lines], " ");

		line_sz -= gs_string_length(token) + 1;

		token = strtok(NULL, " ");
	}

	(*num_lines)++;
}

#endif // MG_RENDER_H