/*================================================================
	* graphics/ui_manager.c
	*
	* Copyright (c) 2021 Lauri Räsänen
	* ================================

	...
=================================================================*/

#include "ui_manager.h"
#include "../game/console.h"
#include "../game/game_manager.h"
#include "../game/time_manager.h"
#include "../util/render.h"
#include "renderer.h"

mg_ui_manager_t *g_ui_manager;

void mg_ui_manager_init()
{
	// Allocate
	g_ui_manager = gs_malloc_init(mg_ui_manager_t);

	// Load fonts
#ifdef __ANDROID__
	gs_asset_font_load_from_file("./assets/fonts/PixeloidSans.otf", &g_ui_manager->fonts[GUI_FONT_SMALL], 28);
	gs_asset_font_load_from_file("./assets/fonts/PixeloidSans.otf", &g_ui_manager->fonts[GUI_FONT_MEDIUM], 36);
	gs_asset_font_load_from_file("./assets/fonts/PixeloidSans.otf", &g_ui_manager->fonts[GUI_FONT_LARGE], 48);
#else
	gs_asset_font_load_from_file("./assets/fonts/PixeloidSans.otf", &g_ui_manager->fonts[GUI_FONT_SMALL], 14);
	gs_asset_font_load_from_file("./assets/fonts/PixeloidSans.otf", &g_ui_manager->fonts[GUI_FONT_MEDIUM], 18);
	gs_asset_font_load_from_file("./assets/fonts/PixeloidSans.otf", &g_ui_manager->fonts[GUI_FONT_LARGE], 24);
#endif

	// Default style sheet
	gs_gui_style_element_t panel_style[] = {
		{.type = GS_GUI_STYLE_FONT, .font = &g_ui_manager->fonts[GUI_FONT_SMALL]},
		{.type = GS_GUI_STYLE_PADDING_TOP, .value = 20},
		{.type = GS_GUI_STYLE_COLOR_BORDER, .color = gs_color(0, 0, 0, 0)},
		{.type = GS_GUI_STYLE_COLOR_BACKGROUND, .color = gs_color(0, 0, 0, 0)},
	};

	gs_gui_style_element_t button_style[] = {
		{.type = GS_GUI_STYLE_FONT, .font = &g_ui_manager->fonts[GUI_FONT_SMALL]},
		{.type = GS_GUI_STYLE_ALIGN_CONTENT, .value = GS_GUI_ALIGN_CENTER},
		{.type = GS_GUI_STYLE_JUSTIFY_CONTENT, .value = GS_GUI_JUSTIFY_CENTER},
		{.type = GS_GUI_STYLE_WIDTH, .value = 200},
		{.type = GS_GUI_STYLE_HEIGHT, .value = 50},
		{.type = GS_GUI_STYLE_MARGIN_LEFT, .value = 0},
		{.type = GS_GUI_STYLE_MARGIN_TOP, .value = 10},
		{.type = GS_GUI_STYLE_MARGIN_BOTTOM, .value = 0},
		{.type = GS_GUI_STYLE_MARGIN_RIGHT, .value = 20},
		{.type = GS_GUI_STYLE_SHADOW_X, .value = 1},
		{.type = GS_GUI_STYLE_SHADOW_Y, .value = 1},
		{.type = GS_GUI_STYLE_COLOR_SHADOW, .color = gs_color(146, 146, 146, 200)},
		{.type = GS_GUI_STYLE_COLOR_BORDER, .color = GS_COLOR_BLACK},
		{.type = GS_GUI_STYLE_BORDER_WIDTH, .value = 2},
		{.type = GS_GUI_STYLE_COLOR_CONTENT, .color = gs_color(67, 67, 67, 255)},
		{.type = GS_GUI_STYLE_COLOR_BACKGROUND, .color = gs_color(198, 198, 198, 255)},
	};

	gs_gui_style_element_t button_hover_style[] = {
		{.type = GS_GUI_STYLE_FONT, .font = &g_ui_manager->fonts[GUI_FONT_SMALL]},
		{.type = GS_GUI_STYLE_COLOR_BACKGROUND, .color = gs_color(168, 168, 168, 255)},
	};

	gs_gui_style_element_t button_focus_style[] = {
		{.type = GS_GUI_STYLE_FONT, .font = &g_ui_manager->fonts[GUI_FONT_SMALL]},
		{.type = GS_GUI_STYLE_COLOR_CONTENT, .color = gs_color(255, 255, 255, 255)},
		{.type = GS_GUI_STYLE_COLOR_BACKGROUND, .color = gs_color(49, 174, 31, 255)},
	};

	gs_gui_style_element_t label_style[] = {
		{.type = GS_GUI_STYLE_FONT, .font = &g_ui_manager->fonts[GUI_FONT_SMALL]},
		{.type = GS_GUI_STYLE_ALIGN_CONTENT, .value = GS_GUI_ALIGN_CENTER},
		{.type = GS_GUI_STYLE_JUSTIFY_CONTENT, .value = GS_GUI_JUSTIFY_END},
	};

	gs_gui_style_element_t text_style[] = {
		{.type = GS_GUI_STYLE_FONT, .font = &g_ui_manager->fonts[GUI_FONT_SMALL]},
		{.type = GS_GUI_STYLE_ALIGN_CONTENT, .value = GS_GUI_ALIGN_CENTER},
		{.type = GS_GUI_STYLE_JUSTIFY_CONTENT, .value = GS_GUI_JUSTIFY_START},
	};

	g_ui_manager->default_style_sheet = gs_gui_style_sheet_create(
		&g_renderer->gui,
		&(gs_gui_style_sheet_desc_t){
			.button = {
				.all   = {button_style, sizeof(button_style)},
				.hover = {button_hover_style, sizeof(button_hover_style)},
				.focus = {button_focus_style, sizeof(button_focus_style)},
			},
			.panel = {
				.all = {panel_style, sizeof(panel_style)},
			},
			.label = {
				.all = {label_style, sizeof(label_style)},
			},
			.text = {
				.all = {text_style, sizeof(text_style)},
			},
		});

	// Console style sheet
	gs_gui_style_element_t console_text_style[] = {
		{.type = GS_GUI_STYLE_FONT, .font = &g_ui_manager->fonts[GUI_FONT_SMALL]},
		{.type = GS_GUI_STYLE_ALIGN_CONTENT, .value = GS_GUI_ALIGN_START},
		{.type = GS_GUI_STYLE_JUSTIFY_CONTENT, .value = GS_GUI_JUSTIFY_START},
	};

	g_ui_manager->console_style_sheet = gs_gui_style_sheet_create(
		&g_renderer->gui,
		&(gs_gui_style_sheet_desc_t){
			.panel = {
				.all = {panel_style, sizeof(panel_style)},
			},
			.text = {
				.all = {console_text_style, sizeof(console_text_style)},
			},
		});

	// Dialogue style sheet
	gs_gui_style_element_t dialogue_text_style[] = {
		{.type = GS_GUI_STYLE_FONT, .font = &g_ui_manager->fonts[GUI_FONT_MEDIUM]},
		{.type = GS_GUI_STYLE_ALIGN_CONTENT, .value = GS_GUI_ALIGN_START},
		{.type = GS_GUI_STYLE_JUSTIFY_CONTENT, .value = GS_GUI_JUSTIFY_START},
	};

	gs_gui_style_element_t dialogue_panel_style[] = {
		{.type = GS_GUI_STYLE_FONT, .font = &g_ui_manager->fonts[GUI_FONT_MEDIUM]},
		{.type = GS_GUI_STYLE_COLOR_BORDER, .color = gs_color(0, 0, 0, 0)},
		{.type = GS_GUI_STYLE_COLOR_BACKGROUND, .color = gs_color(0, 0, 0, 0)},
	};

	g_ui_manager->dialogue_style_sheet = gs_gui_style_sheet_create(
		&g_renderer->gui,
		&(gs_gui_style_sheet_desc_t){
			.panel = {
				.all = {dialogue_panel_style, sizeof(dialogue_panel_style)},
			},
			.text = {
				.all = {dialogue_text_style, sizeof(dialogue_text_style)},
			},
		});
}

void mg_ui_manager_free()
{
	for (size_t i = 0; i < GUI_FONT_COUNT; i++)
	{
		if (g_ui_manager->fonts[i].font_info)
		{
			gs_free(g_ui_manager->fonts[i].font_info);
			g_ui_manager->fonts[i].font_info = NULL;
		}

		for (size_t j = 0; j < GS_GRAPHICS_TEXTURE_DATA_MAX; j++)
		{
			if (g_ui_manager->fonts[i].texture.desc.data[j])
			{
				gs_free(g_ui_manager->fonts[i].texture.desc.data[j]);
				g_ui_manager->fonts[i].texture.desc.data[j] = NULL;
			}
		}

		if (gs_handle_is_valid(g_ui_manager->fonts[i].texture.hndl))
		{
			gs_graphics_texture_destroy(g_ui_manager->fonts[i].texture.hndl);
			g_ui_manager->fonts[i].texture.hndl = gs_handle_invalid(gs_graphics_texture_t);
		}
	}

	mg_ui_manager_clear_text();
	gs_slot_array_free(g_ui_manager->texts);

	gs_free(g_ui_manager->current_dialogue.content);
	gs_free(g_ui_manager);
	g_ui_manager = NULL;
}

void mg_ui_manager_render(gs_vec2 fbs, bool32_t clear)
{
	bool show_cursor_prev = g_ui_manager->show_cursor;

	g_ui_manager->show_cursor = g_ui_manager->menu_open || g_ui_manager->console_open;

	if (g_ui_manager->show_cursor != show_cursor_prev)
	{
		if (g_ui_manager->show_cursor)
		{
			gs_platform_lock_mouse(gs_platform_main_window(), false);
			gs_platform_mouse_set_position(gs_platform_main_window(), fbs.x * 0.5f, fbs.y * 0.5f);
		}
		else
		{
			gs_platform_lock_mouse(gs_platform_main_window(), true);
		}
	}

	double pt = gs_platform_elapsed_time();

	// Begin new frame for gui
	gs_gui_begin(&g_renderer->gui, g_renderer->fb_size);
	{
		// Set style sheet
		gs_gui_set_style_sheet(&g_renderer->gui, &g_ui_manager->default_style_sheet);

		if (gs_gui_window_begin_ex(
			    &g_renderer->gui,
			    "#root",
			    gs_gui_rect(0, 0, 0, 0),
			    NULL,
			    NULL,
			    GS_GUI_OPT_NOFRAME |
				    GS_GUI_OPT_NOTITLE |
				    GS_GUI_OPT_NOMOVE |
				    GS_GUI_OPT_FULLSCREEN |
				    GS_GUI_OPT_NORESIZE |
				    GS_GUI_OPT_NODOCK |
				    GS_GUI_OPT_NOBRINGTOFRONT))
		{
			gs_gui_container_t *root = gs_gui_get_current_container(&g_renderer->gui);
			_mg_ui_manager_text_overlay(fbs, root);
			_mg_ui_manager_dialogue_window(fbs, root);
			_mg_ui_manager_menu_window(fbs, root);
			_mg_ui_manager_debug_overlay(fbs, root);
			_mg_ui_manager_console_window(fbs, root);

			gs_gui_window_end(&g_renderer->gui);
		}

		// End gui frame
		gs_gui_end(&g_renderer->gui);

		// Do rendering
		gs_graphics_renderpass_begin(&g_renderer->cb, (gs_handle(gs_graphics_renderpass_t)){0});
		if (clear)
		{
			gs_graphics_clear_desc_t clear = (gs_graphics_clear_desc_t){
				.actions = &(gs_graphics_clear_action_t){
					.color = {
						g_renderer->clear_color[0],
						g_renderer->clear_color[1],
						g_renderer->clear_color[2],
						g_renderer->clear_color[3],
					},
				},
			};
			gs_graphics_clear(&g_renderer->cb, &clear);
		}
		gs_gui_render(&g_renderer->gui, &g_renderer->cb);
		gs_graphics_renderpass_end(&g_renderer->cb);
	}
}

void mg_ui_manager_set_dialogue(const char *text, float32_t duration)
{
	gs_free(g_ui_manager->current_dialogue.content);

	mg_ui_dialogue_t diag = {
		.content     = gs_malloc(gs_string_length(text) + 1),
		.duration    = duration,
		._start_time = gs_platform_elapsed_time(),
	};
	memcpy(diag.content, text, gs_string_length(text) + 1);

	g_ui_manager->current_dialogue = diag;
	g_ui_manager->dialogue_open    = true;
}

// Use sz > 0 to reserve a longer string for future calls to mg_ui_manager_update_text.
uint32_t mg_ui_manager_add_text(const char *text, const gs_vec2 pos, size_t sz)
{
	if (sz <= 0) sz = gs_string_length(text) + 1;

	mg_ui_text_t t = {
		.content = gs_malloc(sz),
		.pos	 = pos,
		.sz	 = sz,
	};
	memcpy(t.content, text, sz);
	return gs_slot_array_insert(g_ui_manager->texts, t);
}

void mg_ui_manager_update_text(const uint32_t id, const char *text)
{
	if (gs_slot_array_handle_valid(g_ui_manager->texts, id))
	{
		mg_ui_text_t t = gs_slot_array_get(g_ui_manager->texts, id);
		size_t sz_old  = t.sz;
		size_t sz_new  = gs_string_length(text) + 1;
		if (sz_new > sz_old)
		{
			mg_println("WARN: mg_ui_manager_update_text new text size larger than old (%zu > %zu)", sz_new, sz_old);
			memcpy(t.content, text, sz_old);
			// Ensure null-terminated since we are clipping new string
			memset(t.content + sz_old - 1, '\0', 1);
		}
		else
		{
			memcpy(t.content, text, sz_new);
		}
	}
}

void mg_ui_manager_remove_text(const uint32_t id)
{
	if (gs_slot_array_handle_valid(g_ui_manager->texts, id))
	{
		mg_ui_text_t t = gs_slot_array_get(g_ui_manager->texts, id);
		gs_free(t.content);
		gs_slot_array_erase(g_ui_manager->texts, id);
	}
}

void mg_ui_manager_clear_text()
{
	for (
		gs_slot_array_iter it = gs_slot_array_iter_new(g_ui_manager->texts);
		gs_slot_array_iter_valid(g_ui_manager->texts, it);
		gs_slot_array_iter_advance(g_ui_manager->texts, it))
	{
		mg_ui_text_t text = gs_slot_array_iter_get(g_ui_manager->texts, it);
		gs_free(text.content);
	}
	gs_slot_array_clear(g_ui_manager->texts);
}

void _mg_ui_manager_text_overlay(gs_vec2 fbs, gs_gui_container_t *root)
{
	if (gs_slot_array_size(g_ui_manager->texts) <= 0) return;

	gs_gui_set_style_sheet(&g_renderer->gui, &g_ui_manager->console_style_sheet);
	gs_gui_layout_set_next(&g_renderer->gui, gs_gui_layout_anchor(&root->body, fbs.x, fbs.y, 0, 0, GS_GUI_LAYOUT_ANCHOR_TOPLEFT), 0);
	gs_gui_panel_begin_ex(&g_renderer->gui, "#texts", NULL, GS_GUI_OPT_NOSCROLL);
	{
		gs_gui_container_t *cnt = gs_gui_get_current_container(&g_renderer->gui);
		gs_gui_rect_t next	= {};

		for (
			gs_slot_array_iter it = gs_slot_array_iter_new(g_ui_manager->texts);
			gs_slot_array_iter_valid(g_ui_manager->texts, it);
			gs_slot_array_iter_advance(g_ui_manager->texts, it))
		{
			mg_ui_text_t text = gs_slot_array_iter_get(g_ui_manager->texts, it);
			gs_gui_layout_set_next(&g_renderer->gui, gs_gui_layout_anchor(&cnt->body, fbs.x, fbs.y, text.pos.x, text.pos.y, GS_GUI_LAYOUT_ANCHOR_TOPLEFT), 0);
			next = gs_gui_layout_next(&g_renderer->gui);
			gs_gui_draw_control_text(&g_renderer->gui, text.content, next, &g_ui_manager->console_style_sheet.styles[GS_GUI_ELEMENT_TEXT][GS_GUI_ELEMENT_STATE_DEFAULT], 0x00);
		}
	}
	gs_gui_panel_end(&g_renderer->gui);
}

void _mg_ui_manager_debug_overlay(gs_vec2 fbs, gs_gui_container_t *root)
{
	if (!g_ui_manager->debug_open) return;

	char tmp[64];

	gs_gui_set_style_sheet(&g_renderer->gui, &g_ui_manager->console_style_sheet);
	gs_gui_layout_set_next(&g_renderer->gui, gs_gui_layout_anchor(&root->body, fbs.x, fbs.y, 0, 0, GS_GUI_LAYOUT_ANCHOR_TOPLEFT), 0);
	gs_gui_panel_begin_ex(&g_renderer->gui, "#debug", NULL, GS_GUI_OPT_NOSCROLL);
	{
		gs_gui_container_t *cnt = gs_gui_get_current_container(&g_renderer->gui);
		gs_gui_rect_t next	= {};

		float tmp_y	      = 5;
		const float tmp_pad   = 3;
		gs_gui_style_t *style = &g_ui_manager->console_style_sheet.styles[GS_GUI_ELEMENT_TEXT][GS_GUI_ELEMENT_STATE_DEFAULT];
		float32_t line_height = gs_asset_font_max_height(style->font);
#define DRAW_TMP(POS_X, POS_Y)                                                                                                                           \
	{                                                                                                                                                \
		gs_gui_layout_set_next(&g_renderer->gui, gs_gui_layout_anchor(&cnt->body, fbs.x, fbs.y, POS_X, POS_Y, GS_GUI_LAYOUT_ANCHOR_TOPLEFT), 0); \
		next = gs_gui_layout_next(&g_renderer->gui);                                                                                             \
		gs_gui_draw_control_text(&g_renderer->gui, tmp, next, style, 0x00);                                                                      \
		tmp_y += line_height + tmp_pad;                                                                                                          \
	}

		// draw fps
		sprintf(
			tmp,
			"fps: %d",
			(int)gs_round(1.0f / gs_platform_delta_time()));
		DRAW_TMP(5, tmp_y)

		// draw times
		sprintf(tmp, "game:");
		DRAW_TMP(5, tmp_y)
		sprintf(tmp, "update: %.2fms", g_time_manager->update);
		DRAW_TMP(10, tmp_y)
		sprintf(tmp, "render: %.2fms", g_time_manager->render);
		DRAW_TMP(10, tmp_y)

		sprintf(tmp, "gs:");
		DRAW_TMP(5, tmp_y)
		sprintf(tmp, "update: %.2fms", gs_platform_time()->update);
		DRAW_TMP(10, tmp_y)
		sprintf(tmp, "render: %.2fms", gs_platform_time()->render);
		DRAW_TMP(10, tmp_y)
		sprintf(tmp, "wait: %.2fms", gs_platform_time()->frame - gs_platform_time()->update - gs_platform_time()->render);
		DRAW_TMP(10, tmp_y)

		// draw map stats
		if (g_renderer->bsp != NULL && g_renderer->bsp->valid)
		{
			sprintf(tmp, "map: %s", g_renderer->bsp->name);
			DRAW_TMP(5, tmp_y)
			sprintf(tmp, "tris: %zu/%zu", g_renderer->bsp->stats.visible_indices / 3, g_renderer->bsp->stats.total_indices / 3);
			DRAW_TMP(10, tmp_y)
			sprintf(tmp, "faces: %zu/%zu", g_renderer->bsp->stats.visible_faces, g_renderer->bsp->stats.total_faces);
			DRAW_TMP(10, tmp_y)
			sprintf(tmp, "patches: %zu/%zu", g_renderer->bsp->stats.visible_patches, g_renderer->bsp->stats.total_patches);
			DRAW_TMP(10, tmp_y)
			sprintf(tmp, "leaf: %zu, cluster: %d", g_renderer->bsp->stats.current_leaf, g_renderer->bsp->leaves.data[g_renderer->bsp->stats.current_leaf].cluster);
			DRAW_TMP(10, tmp_y)
		}

		// draw player stats
		if (g_game_manager != NULL && g_game_manager->player != NULL)
		{
			sprintf(tmp, "player:");
			DRAW_TMP(5, tmp_y)
			sprintf(tmp, "pos: [%f, %f, %f]", g_game_manager->player->transform.position.x, g_game_manager->player->transform.position.y, g_game_manager->player->transform.position.z);
			DRAW_TMP(10, tmp_y)
			sprintf(tmp, "ang: [%f, %f, %f]", g_game_manager->player->yaw, g_game_manager->player->camera.pitch, g_game_manager->player->camera.roll);
			DRAW_TMP(10, tmp_y)
			sprintf(tmp, "vel: [%f, %f, %f]", g_game_manager->player->velocity.x, g_game_manager->player->velocity.y, g_game_manager->player->velocity.z);
			DRAW_TMP(10, tmp_y)
			sprintf(tmp, "vel_abs: %f, h: %f", gs_vec3_len(g_game_manager->player->velocity), gs_vec3_len(gs_v3(g_game_manager->player->velocity.x, g_game_manager->player->velocity.y, 0)));
			DRAW_TMP(10, tmp_y)
		}
		else if (g_renderer->cam)
		{
			// use renderer camera directly
			sprintf(tmp, "camera:");
			DRAW_TMP(5, tmp_y)
			sprintf(tmp, "pos: [%f, %f, %f]", g_renderer->cam->transform.position.x, g_renderer->cam->transform.position.y, g_renderer->cam->transform.position.z);
			DRAW_TMP(10, tmp_y)
			// TODO: yaw/pitch/roll conversion
			// sprintf(tmp, "ang: [%f, %f, %f]", gs_rad2deg(g_renderer->cam->transform.rotation.x), gs_rad2deg(g_renderer->cam->transform.rotation.y), gs_rad2deg(g_renderer->cam->transform.rotation.z));
			// DRAW_TMP(10, tmp_y)
		}
	}
	gs_gui_panel_end(&g_renderer->gui);
}

void _mg_ui_manager_console_window(gs_vec2 fbs, gs_gui_container_t *root)
{
	if (!g_ui_manager->console_open) return;

	char tmp[64];

	gs_gui_set_style_sheet(&g_renderer->gui, &g_ui_manager->console_style_sheet);
	gs_gui_layout_set_next(&g_renderer->gui, gs_gui_layout_anchor(&root->body, fbs.x, fbs.y, 0, 0, GS_GUI_LAYOUT_ANCHOR_TOPLEFT), 0);
	gs_gui_panel_begin_ex(&g_renderer->gui, "#console", NULL, GS_GUI_OPT_NOSCROLL);
	{
		gs_gui_container_t *con = gs_gui_get_current_container(&g_renderer->gui);
		gs_gui_rect_t next	= {};

		// draw background
		gs_gui_layout_set_next(&g_renderer->gui, gs_gui_layout_anchor(&con->body, fbs.x, fbs.y, 0, 0, GS_GUI_LAYOUT_ANCHOR_TOPLEFT), 0);
		next = gs_gui_layout_next(&g_renderer->gui);
		gs_gui_draw_rect(&g_renderer->gui, next, gs_color(0, 0, 0, 200));
		gs_gui_rect_t bg = g_renderer->gui.last_rect;

#define DRAW_CON(TEXT, POS_X, POS_Y)                                                                                                                                        \
	{                                                                                                                                                                   \
		gs_gui_layout_set_next(&g_renderer->gui, gs_gui_layout_anchor(&con->body, fbs.x, fbs.y, POS_X, POS_Y, GS_GUI_LAYOUT_ANCHOR_TOPLEFT), 0);                    \
		next = gs_gui_layout_next(&g_renderer->gui);                                                                                                                \
		gs_gui_draw_control_text(&g_renderer->gui, TEXT, next, &g_ui_manager->console_style_sheet.styles[GS_GUI_ELEMENT_TEXT][GS_GUI_ELEMENT_STATE_DEFAULT], 0x00); \
	}

		gs_asset_font_t *font = g_ui_manager->console_style_sheet.styles[GS_GUI_ELEMENT_TEXT][GS_GUI_ELEMENT_STATE_DEFAULT].font;
		float32_t line_height = gs_asset_font_max_height(font);
		float32_t char_width  = gs_asset_font_text_dimensions(font, "W", 1).x;

		// Draw output
		int32_t line_num     = 0;
		int32_t input_height = line_height + 16;
		int32_t line_offset  = input_height + line_height + 4;
		for (size_t i = 0; i < MG_CON_LINES; i++)
		{
			if (g_console->output[i] == NULL)
			{
				continue;
			}

			if (line_num >= g_ui_manager->console_scroll_y)
			{
				DRAW_CON(
					g_console->output[i],
					g_ui_manager->console_scroll_x * char_width,
					fbs.y - line_offset);
				line_offset += line_height;
			}

			line_num++;
		}

		// Draw input
		gs_gui_layout_set_next(&g_renderer->gui, gs_gui_layout_anchor(&con->body, fbs.x, input_height, 0, 0, GS_GUI_LAYOUT_ANCHOR_BOTTOMLEFT), 0);
		gs_gui_panel_begin_ex(&g_renderer->gui, "#console-input", NULL, GS_GUI_OPT_NOSCROLL);
		{
			gs_gui_layout_row(&g_renderer->gui, 1, (int[]){-1}, input_height);
			gs_gui_textbox(&g_renderer->gui, g_ui_manager->console_input, 256);
		}
		gs_gui_panel_end(&g_renderer->gui);
	}
	gs_gui_panel_end(&g_renderer->gui);
}

void _mg_ui_manager_dialogue_window(gs_vec2 fbs, gs_gui_container_t *root)
{
	if (!g_ui_manager->dialogue_open) return;

	mg_ui_dialogue_t diag = g_ui_manager->current_dialogue;

	gs_gui_set_style_sheet(&g_renderer->gui, &g_ui_manager->dialogue_style_sheet);

	gs_gui_layout_set_next(&g_renderer->gui, gs_gui_layout_anchor(&root->body, fbs.x, fbs.y, 0, 0, GS_GUI_LAYOUT_ANCHOR_CENTER), 0);
	gs_gui_panel_begin_ex(&g_renderer->gui, "#dialogue", NULL, GS_GUI_OPT_NOSCROLL);
	{
		gs_gui_container_t *dialogue = gs_gui_get_current_container(&g_renderer->gui);

		// split text to lines
		uint32_t max_width    = 600;
		int32_t offset_y      = -50;
		gs_asset_font_t *font = g_ui_manager->dialogue_style_sheet.styles[GS_GUI_ELEMENT_TEXT][GS_GUI_ELEMENT_STATE_DEFAULT].font;
		float32_t line_height = gs_asset_font_max_height(font);
		uint32_t *num_lines   = gs_malloc_init(uint32_t);
		char **lines	      = gs_malloc(sizeof(char *) * 64);
		mg_text_to_lines(font, diag.content, max_width, lines, num_lines);
		float32_t pad_y	 = 1.0f * line_height;
		float32_t pad_x	 = 2.0f * gs_asset_font_text_dimensions(font, " ", -1).x;
		float32_t height = (*num_lines) * line_height;

		// draw background
		gs_gui_layout_set_next(&g_renderer->gui, gs_gui_layout_anchor(&dialogue->body, max_width + 2.0f * pad_x, height + 2.0f * pad_y, 0, offset_y, GS_GUI_LAYOUT_ANCHOR_BOTTOMCENTER), 0);
		gs_gui_rect_t next = gs_gui_layout_next(&g_renderer->gui);
		gs_gui_draw_rect(&g_renderer->gui, next, gs_color(0, 0, 0, 100));
		gs_gui_rect_t bg = g_renderer->gui.last_rect;

		// draw lines
		for (size_t i = 0; i < *num_lines; i++)
		{
			float32_t magic = -0.4f * line_height; // texts are too low by roughly this much, TODO: why?
			float32_t off_y = i * line_height + pad_y + magic;
			gs_gui_layout_set_next(&g_renderer->gui, gs_gui_layout_anchor(&dialogue->body, max_width, line_height, bg.x + pad_x, bg.y + off_y, GS_GUI_LAYOUT_ANCHOR_TOPLEFT), 0);
			gs_gui_rect_t next = gs_gui_layout_next(&g_renderer->gui);
			gs_gui_draw_control_text(&g_renderer->gui, lines[i], next, &g_ui_manager->dialogue_style_sheet.styles[GS_GUI_ELEMENT_TEXT][GS_GUI_ELEMENT_STATE_DEFAULT], 0x00);

			gs_free(lines[i]);
		}

		gs_free(lines);
		gs_free(num_lines);
	}
	gs_gui_panel_end(&g_renderer->gui);

	double pt = gs_platform_elapsed_time();
	if (diag.duration > 0 && pt - diag._start_time > diag.duration * 1000)
		g_ui_manager->dialogue_open = false;
}

void _mg_ui_manager_menu_window(gs_vec2 fbs, gs_gui_container_t *root)
{
	if (!g_ui_manager->menu_open) return;

	gs_gui_set_style_sheet(&g_renderer->gui, &g_ui_manager->default_style_sheet);

	gs_gui_layout_set_next(&g_renderer->gui, gs_gui_layout_anchor(&root->body, fbs.x, fbs.y, 0, 0, GS_GUI_LAYOUT_ANCHOR_CENTER), 0);
	gs_gui_panel_begin_ex(&g_renderer->gui, "#menu", NULL, GS_GUI_OPT_NOSCROLL);
	{
		gs_gui_container_t *menu = gs_gui_get_current_container(&g_renderer->gui);

		// buttons panel
		gs_gui_layout_set_next(&g_renderer->gui, gs_gui_layout_anchor(&menu->body, 500, 500, 0, 0, GS_GUI_LAYOUT_ANCHOR_BOTTOMCENTER), 0);
		gs_gui_panel_begin_ex(&g_renderer->gui, "#buttons", NULL, GS_GUI_OPT_NOSCROLL);
		{
			gs_gui_layout_row(&g_renderer->gui, 1, (int[]){-1}, 0);
			_mg_ui_manager_custom_button("Test 1");
			_mg_ui_manager_custom_button("Test 2");
			_mg_ui_manager_custom_button("Test 3");
			if (_mg_ui_manager_custom_button("Exit Game"))
				gs_quit();
		}
		gs_gui_panel_end(&g_renderer->gui);
	}
	gs_gui_panel_end(&g_renderer->gui);
}

// Simple custom button command that allows us to render some inner highlights and shadows
bool _mg_ui_manager_custom_button(const char *str)
{
	gs_gui_context_t *ctx = &g_renderer->gui;
	bool ret	      = gs_gui_button(ctx, str);
	gs_gui_rect_t rect    = ctx->last_rect;

	// Draw inner shadows/highlights over button
	gs_color_t hc = GS_COLOR_WHITE, sc = gs_color(85, 85, 85, 255);
	int32_t w = 2;
	gs_gui_draw_rect(ctx, gs_gui_rect(rect.x + w, rect.y, rect.w - 2 * w, w), hc);
	gs_gui_draw_rect(ctx, gs_gui_rect(rect.x + w, rect.y + rect.h - w, rect.w - 2 * w, w), sc);
	gs_gui_draw_rect(ctx, gs_gui_rect(rect.x, rect.y, w, rect.h), hc);
	gs_gui_draw_rect(ctx, gs_gui_rect(rect.x + rect.w - w, rect.y, w, rect.h), sc);

	return ret;
}