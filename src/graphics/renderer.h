/*================================================================
	* graphics/renderer.h
	*
	* Copyright (c) 2021 Lauri Räsänen
	* ================================

	...
=================================================================*/

#ifndef MG_RENDERER_H
#define MG_RENDERER_H

// clang-format off
#include <gs/gs.h>
#include <gs/util/gs_idraw.h>
#include <gs/util/gs_gui.h>
// clang-format on

#include "../bsp/bsp_map.h"
#include "../entities/player.h"
#include "model_manager.h"
#include "types.h"

typedef enum mg_model_type
{
	MG_MODEL_WORLD,
	MG_MODEL_VIEWMODEL,
	MG_MODEL_COUNT,
} mg_model_type;

// Renderable instance of a model
typedef struct mg_renderable_t
{
	bool hidden;
	mg_model_type type;
	gs_vqs *transform;
	gs_mat4 u_view;
	mg_model_t model;
	mg_md3_animation_t *current_animation;
	int32_t frame;
	double prev_frame_time;
} mg_renderable_t;

typedef struct mg_renderer_t
{
	gs_command_buffer_t cb;
	gs_immediate_draw_t gsi;
	gs_gui_context_t gui;
	gs_camera_t *cam;
	gs_slot_array(mg_renderable_t) renderables;
	gs_handle(gs_graphics_pipeline_t) pipe;
	gs_handle(gs_graphics_pipeline_t) viewmodel_pipe;
	gs_handle(gs_graphics_pipeline_t) wire_pipe;
	gs_handle(gs_graphics_pipeline_t) post_pipe;
	gs_dyn_array(gs_handle(gs_graphics_shader_t)) shaders;
	gs_dyn_array(char *) shader_names;
	gs_dyn_array(char *) shader_sources_vert;
	gs_dyn_array(char *) shader_sources_frag;
	gs_vec2 fb_size;
	int32_t *screen_indices;
	gs_vec2 *screen_vertices;
	bool32_t offscreen_cleared;
	gs_handle(gs_graphics_vertex_buffer_t) screen_vbo;
	gs_handle(gs_graphics_index_buffer_t) screen_ibo;
	gs_handle(gs_graphics_renderpass_t) offscreen_rp;
	gs_handle(gs_graphics_framebuffer_t) offscreen_fbo;
	gs_handle(gs_graphics_texture_t) offscreen_rt;
	gs_handle(gs_graphics_texture_t) offscreen_dt;
	gs_handle(gs_graphics_renderpass_t) viewmodel_rp;
	gs_handle(gs_graphics_framebuffer_t) viewmodel_fbo;
	gs_handle(gs_graphics_texture_t) viewmodel_rt;
	gs_handle(gs_graphics_texture_t) viewmodel_dt;
	gs_handle(gs_graphics_uniform_t) u_proj;
	gs_handle(gs_graphics_uniform_t) u_view;
	gs_handle(gs_graphics_uniform_t) u_light;
	gs_handle(gs_graphics_uniform_t) u_tex;
	gs_handle(gs_graphics_uniform_t) u_tex_vm;
	gs_handle(gs_graphics_uniform_t) u_color;
	gs_handle(gs_graphics_uniform_t) u_barrel_enabled;
	gs_handle(gs_graphics_uniform_t) u_barrel_strength;
	gs_handle(gs_graphics_uniform_t) u_barrel_height;
	gs_handle(gs_graphics_uniform_t) u_barrel_aspect;
	gs_handle(gs_graphics_uniform_t) u_barrel_cyl_ratio;
	gs_handle(gs_graphics_texture_t) missing_texture;
	float clear_color[4];
	float clear_color_overlay[4];
} mg_renderer_t;

void mg_renderer_init(uint32_t window_handle);
void mg_renderer_update();
void mg_renderer_free();
uint32_t mg_renderer_create_renderable(mg_model_t model, gs_vqs *transform);
void mg_renderer_remove_renderable(uint32_t renderable_id);
mg_renderable_t *mg_renderer_get_renderable(uint32_t renderable_id);
gs_handle(gs_graphics_shader_t) mg_renderer_get_shader(char *name);
bool32_t mg_renderer_play_animation(uint32_t id, char *name);
void mg_renderer_set_hidden(uint32_t id, bool hidden);
void mg_renderer_set_model_type(uint32_t id, mg_model_type type);
void _mg_renderer_resize(const gs_vec2 fb);
void _mg_renderer_models_pass();
void _mg_renderer_viewmodel_pass();
void _mg_renderer_post_pass();
void _mg_renderer_immediate_pass();
void _mg_renderer_draw_debug_overlay();
void _mg_renderer_load_shader(char *name);

extern mg_renderer_t *g_renderer;

#endif // MG_RENDERER_H