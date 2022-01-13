/*================================================================
	* graphics/ui_manager.h
	*
	* Copyright (c) 2021 Lauri Räsänen
	* ================================

	...
=================================================================*/

#ifndef MG_UI_MANAGER_H
#define MG_UI_MANAGER_H

// clang-format off
#include <gs/gs.h>
#include <gs/util/gs_idraw.h>
#include <gs/util/gs_gui.h>
// clang-format on

#include "model.h"

enum
{
	GUI_STYLE_ROOT = 0x00,
	GUI_STYLE_COUNT
};

enum
{
	GUI_FONT_DEFAULT = 0x00,
	GUI_FONT_COUNT
};

typedef struct mg_ui_element_t
{
	uint32_t width;
	uint32_t height;
	gs_vec2 position;
	bool32_t center_x;
	bool32_t center_y;
	float32_t duration;
	double _start_time;
} mg_ui_element_t;

typedef struct mg_ui_text_t
{
	mg_ui_element_t element;
	char *content;
	float32_t font_size;
	gs_color_t color;
} mg_ui_text_t;

typedef struct mg_ui_rect_t
{
	mg_ui_element_t element;
	gs_color_t color;
} mg_ui_rect_t;

typedef struct mg_ui_manager_t
{
	gs_gui_style_t styles[GUI_STYLE_COUNT][GS_GUI_ELEMENT_STATE_COUNT];
	gs_asset_font_t fonts[GUI_FONT_COUNT];
	gs_gui_style_sheet_t default_style_sheet;
	gs_gui_style_sheet_t console_style_sheet;
	bool menu_open;
	bool debug_open;
	bool console_open;
	bool show_cursor;
} mg_ui_manager_t;

void mg_ui_manager_init();
void mg_ui_manager_free();

void mg_ui_manager_render(gs_vec2 fbs);

void mg_ui_manager_add_dialogue(char *text, float32_t duration);

void _mg_ui_manager_debug_overlay(gs_vec2 fbs);

void _mg_ui_manager_menu_window(gs_vec2 fbs);

bool _mg_ui_manager_custom_button(const char *str);

extern mg_ui_manager_t *g_ui_manager;

#endif // MG_UI_MANAGER_H