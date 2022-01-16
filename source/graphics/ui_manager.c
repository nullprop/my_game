/*================================================================
	* graphics/ui_manager.c
	*
	* Copyright (c) 2021 Lauri Räsänen
	* ================================

	...
=================================================================*/

#include "ui_manager.h"
#include "renderer.h"

mg_ui_manager_t *g_ui_manager;

void mg_ui_manager_init()
{
	// Allocate
	g_ui_manager = gs_malloc_init(mg_ui_manager_t);

	// Load fonts
	gs_asset_font_load_from_file("./fonts/PixeloidSans.otf", &g_ui_manager->fonts[GUI_FONT_SMALL], 14);
	gs_asset_font_load_from_file("./fonts/PixeloidSans.otf", &g_ui_manager->fonts[GUI_FONT_MEDIUM], 18);
	gs_asset_font_load_from_file("./fonts/PixeloidSans.otf", &g_ui_manager->fonts[GUI_FONT_LARGE], 24);

	// Default style sheet
	gs_gui_style_element_t panel_style[] = {
		{GS_GUI_STYLE_FONT, .data = &g_ui_manager->fonts[GUI_FONT_SMALL]},
		{GS_GUI_STYLE_PADDING_TOP, 20},
		{GS_GUI_STYLE_BORDER_COLOR, .color = gs_color(0, 0, 0, 0)},
		{GS_GUI_STYLE_BACKGROUND_COLOR, .color = gs_color(0, 0, 0, 0)}};

	gs_gui_style_element_t button_style[] = {
		{GS_GUI_STYLE_FONT, .data = &g_ui_manager->fonts[GUI_FONT_SMALL]},
		{GS_GUI_STYLE_ALIGN_CONTENT, GS_GUI_ALIGN_CENTER},
		{GS_GUI_STYLE_JUSTIFY_CONTENT, GS_GUI_JUSTIFY_CENTER},
		{GS_GUI_STYLE_WIDTH, 200},
		{GS_GUI_STYLE_HEIGHT, 50},
		{GS_GUI_STYLE_MARGIN_LEFT, 0},
		{GS_GUI_STYLE_MARGIN_TOP, 10},
		{GS_GUI_STYLE_MARGIN_BOTTOM, 0},
		{GS_GUI_STYLE_MARGIN_RIGHT, 20},
		{GS_GUI_STYLE_SHADOW_X, 1},
		{GS_GUI_STYLE_SHADOW_Y, 1},
		{GS_GUI_STYLE_SHADOW_COLOR, .color = gs_color(146, 146, 146, 200)},
		{GS_GUI_STYLE_BORDER_COLOR, .color = GS_COLOR_BLACK},
		{GS_GUI_STYLE_BORDER_WIDTH, 2},
		{GS_GUI_STYLE_CONTENT_COLOR, .color = gs_color(67, 67, 67, 255)},
		{GS_GUI_STYLE_BACKGROUND_COLOR, .color = gs_color(198, 198, 198, 255)}};

	gs_gui_style_element_t button_hover_style[] = {
		{GS_GUI_STYLE_FONT, .data = &g_ui_manager->fonts[GUI_FONT_SMALL]},
		{GS_GUI_STYLE_BACKGROUND_COLOR, .color = gs_color(168, 168, 168, 255)}};

	gs_gui_style_element_t button_focus_style[] = {
		{GS_GUI_STYLE_FONT, .data = &g_ui_manager->fonts[GUI_FONT_SMALL]},
		{GS_GUI_STYLE_CONTENT_COLOR, .color = gs_color(255, 255, 255, 255)},
		{GS_GUI_STYLE_BACKGROUND_COLOR, .color = gs_color(49, 174, 31, 255)}};

	gs_gui_style_element_t label_style[] = {
		{GS_GUI_STYLE_FONT, .data = &g_ui_manager->fonts[GUI_FONT_SMALL]},
		{GS_GUI_STYLE_ALIGN_CONTENT, GS_GUI_ALIGN_CENTER},
		{GS_GUI_STYLE_JUSTIFY_CONTENT, GS_GUI_JUSTIFY_END}};

	gs_gui_style_element_t text_style[] = {
		{GS_GUI_STYLE_FONT, .data = &g_ui_manager->fonts[GUI_FONT_SMALL]},
		{GS_GUI_STYLE_ALIGN_CONTENT, GS_GUI_ALIGN_CENTER},
		{GS_GUI_STYLE_JUSTIFY_CONTENT, GS_GUI_JUSTIFY_START}};

	g_ui_manager->default_style_sheet = gs_gui_style_sheet_new(
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
		{GS_GUI_STYLE_FONT, .data = &g_ui_manager->fonts[GUI_FONT_SMALL]},
		{GS_GUI_STYLE_ALIGN_CONTENT, GS_GUI_ALIGN_START},
		{GS_GUI_STYLE_JUSTIFY_CONTENT, GS_GUI_JUSTIFY_START}};

	g_ui_manager->console_style_sheet = gs_gui_style_sheet_new(
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
		{GS_GUI_STYLE_FONT, .data = &g_ui_manager->fonts[GUI_FONT_MEDIUM]},
		{GS_GUI_STYLE_ALIGN_CONTENT, GS_GUI_ALIGN_START},
		{GS_GUI_STYLE_JUSTIFY_CONTENT, GS_GUI_JUSTIFY_START}};

	g_ui_manager->dialogue_style_sheet = gs_gui_style_sheet_new(
		&g_renderer->gui,
		&(gs_gui_style_sheet_desc_t){
			.panel = {
				.all = {panel_style, sizeof(panel_style)},
			},
			.text = {
				.all = {dialogue_text_style, sizeof(dialogue_text_style)},
			},
		});

	// Test
	mg_ui_manager_set_dialogue("Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod tempor incididunt ut labore et dolore magna aliqua.", -1);
}

void mg_ui_manager_free()
{
	gs_free(g_ui_manager);
	g_ui_manager = NULL;
}

void mg_ui_manager_render(gs_vec2 fbs)
{
	bool show_cursor_prev = g_ui_manager->show_cursor;

	if (gs_platform_key_pressed(GS_KEYCODE_ESC)) g_ui_manager->menu_open = !g_ui_manager->menu_open;
	if (gs_platform_key_pressed(GS_KEYCODE_F2)) g_ui_manager->console_open = !g_ui_manager->console_open;
	if (gs_platform_key_pressed(GS_KEYCODE_F3)) g_ui_manager->debug_open = !g_ui_manager->debug_open;

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
	gs_gui_begin(&g_renderer->gui);
	{
		// Set style sheet
		gs_gui_set_style_sheet(&g_renderer->gui, &g_ui_manager->default_style_sheet);

		if (gs_gui_begin_window_ex(
			    &g_renderer->gui,
			    "#root",
			    gs_gui_rect(0, 0, 0, 0),
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
			_mg_ui_manager_dialogue_window(fbs, root);
			_mg_ui_manager_menu_window(fbs, root);
			_mg_ui_manager_debug_overlay(fbs, root);

			gs_gui_end_window(&g_renderer->gui);
		}

		// End gui frame
		gs_gui_end(&g_renderer->gui);

		// Do rendering
		gs_graphics_begin_render_pass(&g_renderer->cb, (gs_handle(gs_graphics_render_pass_t)){0});
		gs_gui_render(&g_renderer->gui, &g_renderer->cb);
		gs_graphics_end_render_pass(&g_renderer->cb);
	}
}

void mg_ui_manager_set_dialogue(const char *text, float32_t duration)
{
	mg_ui_dialogue_t diag = {
		.content     = gs_malloc(gs_string_length(text) + 1),
		.duration    = duration,
		._start_time = gs_platform_elapsed_time(),
	};
	memcpy(diag.content, text, gs_string_length(text) + 1);

	g_ui_manager->current_dialogue = diag;
	g_ui_manager->dialogue_open    = true;
}

void _mg_ui_manager_debug_overlay(gs_vec2 fbs, gs_gui_container_t *root)
{
	if (!g_ui_manager->debug_open) return;

	char tmp[64];

	gs_gui_set_style_sheet(&g_renderer->gui, &g_ui_manager->console_style_sheet);
	gs_gui_layout_set_next(&g_renderer->gui, gs_gui_layout_anchor(&root->body, fbs.x, fbs.y, 0, 0, GS_GUI_LAYOUT_ANCHOR_TOPLEFT), 0);
	gs_gui_begin_panel_ex(&g_renderer->gui, "#debug", GS_GUI_OPT_NOSCROLL);
	{
		gs_gui_container_t *cnt = gs_gui_get_current_container(&g_renderer->gui);
		gs_gui_rect_t next	= {};

#define DRAW_TMP(POS_X, POS_Y)                                                                                                                           \
	{                                                                                                                                                \
		gs_gui_layout_set_next(&g_renderer->gui, gs_gui_layout_anchor(&cnt->body, fbs.x, fbs.y, POS_X, POS_Y, GS_GUI_LAYOUT_ANCHOR_TOPLEFT), 0); \
		next = gs_gui_layout_next(&g_renderer->gui);                                                                                             \
		gs_gui_draw_control_text(&g_renderer->gui, tmp, next, GS_GUI_ELEMENT_TEXT, 0x00, 0x00);                                                  \
	}

		// draw fps
		sprintf(tmp, "fps: %d", (int)gs_round(1.0f / gs_platform_delta_time()));
		DRAW_TMP(5, 5)

		// draw map stats
		if (g_renderer->bsp != NULL && g_renderer->bsp->valid)
		{
			sprintf(tmp, "map: %s", g_renderer->bsp->name);
			DRAW_TMP(5, 20)
			sprintf(tmp, "tris: %zu/%zu", g_renderer->bsp->stats.visible_indices / 3, g_renderer->bsp->stats.total_indices / 3);
			DRAW_TMP(10, 35)
			sprintf(tmp, "faces: %zu/%zu", g_renderer->bsp->stats.visible_faces, g_renderer->bsp->stats.total_faces);
			DRAW_TMP(10, 50)
			sprintf(tmp, "patches: %zu/%zu", g_renderer->bsp->stats.visible_patches, g_renderer->bsp->stats.total_patches);
			DRAW_TMP(10, 65)
			sprintf(tmp, "leaf: %zu, cluster: %d", g_renderer->bsp->stats.current_leaf, g_renderer->bsp->leaves.data[g_renderer->bsp->stats.current_leaf].cluster);
			DRAW_TMP(10, 80)
		}

		// draw player stats
		if (g_renderer->player != NULL)
		{
			sprintf(tmp, "player:");
			DRAW_TMP(5, 95)
			sprintf(tmp, "pos: [%f, %f, %f]", g_renderer->player->transform.position.x, g_renderer->player->transform.position.y, g_renderer->player->transform.position.z);
			DRAW_TMP(10, 110)
			sprintf(tmp, "ang: [%f, %f, %f]", g_renderer->player->yaw, g_renderer->player->camera.pitch, g_renderer->player->camera.roll);
			DRAW_TMP(10, 125)
			sprintf(tmp, "vel: [%f, %f, %f]", g_renderer->player->velocity.x, g_renderer->player->velocity.y, g_renderer->player->velocity.z);
			DRAW_TMP(10, 140)
			sprintf(tmp, "vel_abs: %f, h: %f", gs_vec3_len(g_renderer->player->velocity), gs_vec3_len(gs_v3(g_renderer->player->velocity.x, g_renderer->player->velocity.y, 0)));
			DRAW_TMP(10, 155)
		}
	}
	gs_gui_end_panel(&g_renderer->gui);
}

void _mg_ui_manager_dialogue_window(gs_vec2 fbs, gs_gui_container_t *root)
{
	if (!g_ui_manager->dialogue_open) return;

	mg_ui_dialogue_t diag = g_ui_manager->current_dialogue;

	gs_gui_set_style_sheet(&g_renderer->gui, &g_ui_manager->dialogue_style_sheet);

	gs_gui_layout_set_next(&g_renderer->gui, gs_gui_layout_anchor(&root->body, fbs.x, fbs.y, 0, 0, GS_GUI_LAYOUT_ANCHOR_CENTER), 0);
	gs_gui_begin_panel_ex(&g_renderer->gui, "#dialogue", GS_GUI_OPT_NOSCROLL);
	{
		gs_gui_container_t *dialogue = gs_gui_get_current_container(&g_renderer->gui);

		// TODO: split text, get rect height from num of lines, draw word at a time

		gs_gui_layout_set_next(&g_renderer->gui, gs_gui_layout_anchor(&dialogue->body, 600, 50, 0, -50, GS_GUI_LAYOUT_ANCHOR_BOTTOMCENTER), 0);
		gs_gui_rect_t next = gs_gui_layout_next(&g_renderer->gui);

		gs_gui_draw_rect(&g_renderer->gui, next, gs_color(0, 0, 0, 100));
		gs_gui_draw_control_text(&g_renderer->gui, diag.content, g_renderer->gui.last_rect, GS_GUI_ELEMENT_TEXT, 0x00, 0x00);
	}
	gs_gui_end_panel(&g_renderer->gui);

	double pt = gs_platform_elapsed_time();
	if (diag.duration > 0 && pt - diag._start_time > diag.duration * 1000)
		g_ui_manager->dialogue_open = false;
}

void _mg_ui_manager_menu_window(gs_vec2 fbs, gs_gui_container_t *root)
{
	if (!g_ui_manager->menu_open) return;

	gs_gui_set_style_sheet(&g_renderer->gui, &g_ui_manager->default_style_sheet);

	gs_gui_layout_set_next(&g_renderer->gui, gs_gui_layout_anchor(&root->body, fbs.x, fbs.y, 0, 0, GS_GUI_LAYOUT_ANCHOR_CENTER), 0);
	gs_gui_begin_panel_ex(&g_renderer->gui, "#menu", GS_GUI_OPT_NOSCROLL);
	{
		gs_gui_container_t *menu = gs_gui_get_current_container(&g_renderer->gui);

		// buttons panel
		gs_gui_layout_set_next(&g_renderer->gui, gs_gui_layout_anchor(&menu->body, 500, 500, 0, 0, GS_GUI_LAYOUT_ANCHOR_BOTTOMCENTER), 0);
		gs_gui_begin_panel_ex(&g_renderer->gui, "#buttons", GS_GUI_OPT_NOSCROLL);
		{
			gs_gui_layout_row(&g_renderer->gui, 1, (int[]){-1}, 0);
			_mg_ui_manager_custom_button("Test 1");
			_mg_ui_manager_custom_button("Test 2");
			_mg_ui_manager_custom_button("Test 3");
			if (_mg_ui_manager_custom_button("Exit Game"))
				gs_engine_quit();
		}
		gs_gui_end_panel(&g_renderer->gui);
	}
	gs_gui_end_panel(&g_renderer->gui);
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