/*================================================================
	* graphics/ui_manager.h
	*
	* Copyright (c) 2021 Lauri Räsänen
	* ================================

	...
=================================================================*/

#ifndef MG_UI_MANAGER_H
#define MG_UI_MANAGER_H

#include <gs/gs.h>
#include <gs/util/gs_gfxt.h>

#include "model.h"

typedef struct mg_ui_element_t
{
	uint32_t width;
	uint32_t height;
	gs_vec2 position;
	bool32_t center_x;
	bool32_t center_y;
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
	gs_slot_array(mg_ui_text_t) texts;
	gs_slot_array(mg_ui_rect_t) rects;
} mg_ui_manager_t;

void mg_ui_manager_init();
void mg_ui_manager_free();

void mg_ui_manager_render(gs_vec2 fb);
void _mg_ui_manager_pass(gs_vec2 fb);

void mg_ui_manager_add_dialogue(char *text);

uint32_t mg_ui_manager_add_text(mg_ui_text_t text);
void mg_ui_manager_remove_text(uint32_t id);
mg_ui_text_t *mg_ui_manager_get_text(uint32_t id);

uint32_t mg_ui_manager_add_rect(mg_ui_rect_t rect);
void mg_ui_manager_remove_rect(uint32_t id);
mg_ui_rect_t *mg_ui_manager_get_rect(uint32_t id);

void _mg_ui_manager_draw_debug_overlay();

extern mg_ui_manager_t *g_ui_manager;

#endif // MG_UI_MANAGER_H