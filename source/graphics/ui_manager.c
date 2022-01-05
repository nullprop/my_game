/*================================================================
	* graphics/ui_manager.c
	*
	* Copyright (c) 2021 Lauri Räsänen
	* ================================

	...
=================================================================*/

#include "ui_manager.h"
#include "renderer.h"
#include <gs/external/stb/stb_truetype.h>

mg_ui_manager_t *g_ui_manager;

void mg_ui_manager_init()
{
	// Allocate
	g_ui_manager	    = gs_malloc_init(mg_ui_manager_t);
	g_ui_manager->texts = gs_slot_array_new(mg_ui_text_t);
	g_ui_manager->rects = gs_slot_array_new(mg_ui_rect_t);

	// Test
	mg_ui_text_t text = {
		.element = {
			.height	  = 64,
			.width	  = 512,
			.position = {.x = 0.5, .y = 0.85},
			.center_x = true,
			.center_y = true,
		},
		.font_size = 16.0f,
		.content   = "Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod tempor incididunt ut labore et dolore magna aliqua.",
		.color	   = {.r = 255, .g = 255, .b = 255, .a = 255},

	};
	mg_ui_manager_add_text(text);
	mg_ui_rect_t rect = {
		.element = {
			.height	  = 64, // same height for center_y
			.width	  = 512 + 16,
			.position = {.x = 0.5, .y = 0.85},
			.center_x = true,
			.center_y = true,
		},
		.color = {.r = 0, .g = 0, .b = 0, .a = 80},
	};
	mg_ui_manager_add_rect(rect);
}

void mg_ui_manager_free()
{
	gs_slot_array_free(g_ui_manager->texts);
	gs_slot_array_free(g_ui_manager->rects);

	gs_free(g_ui_manager);
	g_ui_manager = NULL;
}

void mg_ui_manager_render(gs_vec2 fb)
{
	gsi_camera2D(&g_renderer->gsi);

	// Rects
	if (gs_slot_array_size(g_ui_manager->rects) > 0)
	{
		for (
			gs_slot_array_iter it = gs_slot_array_iter_new(g_ui_manager->rects);
			gs_slot_array_iter_valid(g_ui_manager->rects, it);
			gs_slot_array_iter_advance(g_ui_manager->rects, it))
		{
			mg_ui_rect_t *rect = gs_slot_array_iter_getp(g_ui_manager->rects, it);

			float pos_x = rect->element.position.x * fb.x;
			float pos_y = rect->element.position.y * fb.y;

			if (rect->element.center_x)
				pos_x -= 0.5f * rect->element.width;

			if (rect->element.center_y)
				pos_y -= 0.5f * rect->element.height;

			gsi_rect(
				&g_renderer->gsi,
				pos_x,
				pos_y,
				pos_x + rect->element.width,
				pos_y + rect->element.height,
				rect->color.r,
				rect->color.g,
				rect->color.b,
				rect->color.a,
				GS_GRAPHICS_PRIMITIVE_TRIANGLES);
		}
	}

	_mg_ui_manager_pass(fb);
	gsi_camera2D(&g_renderer->gsi);

	// Texts
	if (gs_slot_array_size(g_ui_manager->texts) > 0)
	{
		gs_asset_font_t *fp	= &g_renderer->gsi.font_default;
		stbtt_bakedchar *glyphs = (stbtt_bakedchar *)fp->glyphs;
		float char_width	= glyphs->xadvance;
		float char_height	= 16.0f;
		char *word;

		for (
			gs_slot_array_iter it = gs_slot_array_iter_new(g_ui_manager->texts);
			gs_slot_array_iter_valid(g_ui_manager->texts, it);
			gs_slot_array_iter_advance(g_ui_manager->texts, it))
		{
			mg_ui_text_t *text = gs_slot_array_iter_getp(g_ui_manager->texts, it);

			char modifiable[gs_string_length(text->content) + 1];
			strcpy(modifiable, text->content);

			float pos_x = text->element.position.x * fb.x;
			// stb_truetype aligns to bottom, move downwards by
			// character height to align with other ui elements.
			float pos_y = text->element.position.y * fb.y + char_height;

			if (text->element.center_x)
				pos_x -= 0.5f * text->element.width;

			if (text->element.center_y)
				pos_y -= 0.5f * text->element.height;

			float cur_pos_x = pos_x;
			float cur_pos_y = pos_y;

			word = strtok(modifiable, " ");
			while (word)
			{
				uint32_t word_width = char_width * gs_string_length(word);

				if (cur_pos_x > pos_x + FLT_EPSILON)
				{
					cur_pos_x += char_width;
					if (cur_pos_x + word_width - pos_x > text->element.width)
					{
						cur_pos_x = pos_x;
						cur_pos_y += char_height;
					}
				}

				gsi_text(
					&g_renderer->gsi,
					cur_pos_x,
					cur_pos_y,
					word,
					NULL,
					false,
					text->color.r,
					text->color.g,
					text->color.b,
					text->color.a);

				cur_pos_x += (float)word_width;
				word = strtok(NULL, " ");
			}
		}
	}

	_mg_ui_manager_draw_debug_overlay();
	_mg_ui_manager_pass(fb);
}

void _mg_ui_manager_pass(gs_vec2 fb)
{
	gs_renderpass im_pass = gs_default_val();
	gs_graphics_begin_render_pass(&g_renderer->cb, im_pass);
	gs_graphics_set_viewport(&g_renderer->cb, 0, 0, (int32_t)fb.x, (int32_t)fb.y);
	gsi_draw(&g_renderer->gsi, &g_renderer->cb);
	gs_graphics_end_render_pass(&g_renderer->cb);
}

uint32_t mg_ui_manager_add_text(mg_ui_text_t text)
{
	return gs_slot_array_insert(g_ui_manager->texts, text);
}

void mg_ui_manager_remove_text(uint32_t id)
{
	gs_slot_array_erase(g_ui_manager->texts, id);
}

mg_ui_text_t *mg_ui_manager_get_text(uint32_t id)
{
	return gs_slot_array_getp(g_ui_manager->texts, id);
}

uint32_t mg_ui_manager_add_rect(mg_ui_rect_t rect)
{
	return gs_slot_array_insert(g_ui_manager->rects, rect);
}

void mg_ui_manager_remove_rect(uint32_t id)
{
	gs_slot_array_erase(g_ui_manager->rects, id);
}

mg_ui_rect_t *mg_ui_manager_get_rect(uint32_t id)
{
	return gs_slot_array_getp(g_ui_manager->rects, id);
}

void _mg_ui_manager_draw_debug_overlay()
{
	// draw fps
	char temp[64];
	sprintf(temp, "fps: %d", (int)gs_round(1.0f / gs_platform_delta_time()));
	gsi_text(&g_renderer->gsi, 5, 15, temp, NULL, false, 255, 255, 255, 255);

	// draw map stats
	if (g_renderer->bsp != NULL && g_renderer->bsp->valid)
	{
		sprintf(temp, "map: %s", g_renderer->bsp->name);
		gsi_text(&g_renderer->gsi, 5, 30, temp, NULL, false, 255, 255, 255, 255);
		sprintf(temp, "tris: %zu/%zu", g_renderer->bsp->stats.visible_indices / 3, g_renderer->bsp->stats.total_indices / 3);
		gsi_text(&g_renderer->gsi, 10, 45, temp, NULL, false, 255, 255, 255, 255);
		sprintf(temp, "faces: %zu/%zu", g_renderer->bsp->stats.visible_faces, g_renderer->bsp->stats.total_faces);
		gsi_text(&g_renderer->gsi, 10, 60, temp, NULL, false, 255, 255, 255, 255);
		sprintf(temp, "patches: %zu/%zu", g_renderer->bsp->stats.visible_patches, g_renderer->bsp->stats.total_patches);
		gsi_text(&g_renderer->gsi, 10, 75, temp, NULL, false, 255, 255, 255, 255);
		sprintf(temp, "leaf: %zu, cluster: %d", g_renderer->bsp->stats.current_leaf, g_renderer->bsp->leaves.data[g_renderer->bsp->stats.current_leaf].cluster);
		gsi_text(&g_renderer->gsi, 10, 90, temp, NULL, false, 255, 255, 255, 255);
	}

	// draw player stats
	if (g_renderer->player != NULL)
	{
		gsi_text(&g_renderer->gsi, 5, 105, "player:", NULL, false, 255, 255, 255, 255);
		sprintf(temp, "pos: [%f, %f, %f]", g_renderer->player->transform.position.x, g_renderer->player->transform.position.y, g_renderer->player->transform.position.z);
		gsi_text(&g_renderer->gsi, 10, 120, temp, NULL, false, 255, 255, 255, 255);
		sprintf(temp, "ang: [%f, %f, %f]", g_renderer->player->yaw, g_renderer->player->camera.pitch, g_renderer->player->camera.roll);
		gsi_text(&g_renderer->gsi, 10, 135, temp, NULL, false, 255, 255, 255, 255);
		sprintf(temp, "vel: [%f, %f, %f]", g_renderer->player->velocity.x, g_renderer->player->velocity.y, g_renderer->player->velocity.z);
		gsi_text(&g_renderer->gsi, 10, 150, temp, NULL, false, 255, 255, 255, 255);
		sprintf(temp, "vel_abs: %f, h: %f", gs_vec3_len(g_renderer->player->velocity), gs_vec3_len(gs_v3(g_renderer->player->velocity.x, g_renderer->player->velocity.y, 0)));
		gsi_text(&g_renderer->gsi, 10, 165, temp, NULL, false, 255, 255, 255, 255);
	}
}