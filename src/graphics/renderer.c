/*================================================================
	* graphics/renderer.c
	*
	* Copyright (c) 2021 Lauri Räsänen
	* ================================

	...
=================================================================*/

#include "renderer.h"
#include "../game/config.h"
#include "../game/console.h"
#include "../game/game_manager.h"
#include "../game/time_manager.h"
#include "../util/camera.h"
#include "../util/render.h"
#include "ui_manager.h"

mg_renderer_t *g_renderer;

void mg_renderer_init(uint32_t window_handle)
{
	// Allocate
	g_renderer	= gs_malloc_init(mg_renderer_t);
	g_renderer->cb	= gs_command_buffer_new();
	g_renderer->gsi = gs_immediate_draw_new(window_handle);
	gs_gui_init(&g_renderer->gui, window_handle);
	g_renderer->renderables		= gs_slot_array_new(mg_renderable_t);
	g_renderer->shaders		= gs_dyn_array_new(gs_handle_gs_graphics_shader_t);
	g_renderer->shader_names	= gs_dyn_array_new(char *);
	g_renderer->shader_sources_frag = gs_dyn_array_new(char *);
	g_renderer->shader_sources_vert = gs_dyn_array_new(char *);

	_mg_renderer_load_shader("basic");
	_mg_renderer_load_shader("basic_unlit");
	_mg_renderer_load_shader("bsp");
	_mg_renderer_load_shader("post");
	_mg_renderer_load_shader("wireframe");
	_mg_renderer_load_shader("bsp_wireframe");

	g_renderer->clear_color[0] = 0;
	g_renderer->clear_color[1] = 0;
	g_renderer->clear_color[2] = 0;
	g_renderer->clear_color[3] = 1.0f;

	// Create offscreen render objects
	g_renderer->offscreen_fbo = gs_graphics_framebuffer_create(NULL);
	g_renderer->offscreen_rt  = gs_handle_invalid(gs_graphics_texture_t);
	g_renderer->offscreen_dt  = gs_handle_invalid(gs_graphics_texture_t);
	_mg_renderer_resize(gs_platform_framebuffer_sizev(gs_platform_main_window()));
	g_renderer->offscreen_rp = gs_graphics_renderpass_create(
		&(gs_graphics_renderpass_desc_t){
			.fbo	    = g_renderer->offscreen_fbo,
			.color	    = &g_renderer->offscreen_rt,
			.color_size = sizeof(g_renderer->offscreen_rt),
			.depth	    = g_renderer->offscreen_dt});

	// Create buffers for full-screen quad
	g_renderer->screen_indices = gs_malloc(sizeof(int32_t) * 6);
	// 1st triangle
	g_renderer->screen_indices[0] = 1;
	g_renderer->screen_indices[1] = 0;
	g_renderer->screen_indices[2] = 3;
	// 2nd triangle
	g_renderer->screen_indices[3] = 1;
	g_renderer->screen_indices[4] = 3;
	g_renderer->screen_indices[5] = 2;
	g_renderer->screen_ibo	      = gs_graphics_index_buffer_create(
		       &(gs_graphics_index_buffer_desc_t){
			       .data = g_renderer->screen_indices,
			       .size = sizeof(int32_t) * 6,
		       });
	g_renderer->screen_vertices    = gs_malloc(sizeof(gs_vec2) * 4);
	g_renderer->screen_vertices[0] = gs_v2(-1.0f, -1.0f);
	g_renderer->screen_vertices[1] = gs_v2(-1.0f, 1.0f);
	g_renderer->screen_vertices[2] = gs_v2(1.0f, 1.0f);
	g_renderer->screen_vertices[3] = gs_v2(1.0f, -1.0f);
	g_renderer->screen_vbo	       = gs_graphics_vertex_buffer_create(
			&(gs_graphics_vertex_buffer_desc_t){
				.data = g_renderer->screen_vertices,
				.size = sizeof(gs_vec2) * 4,
		});

	// Create uniforms
	g_renderer->u_proj = gs_graphics_uniform_create(
		&(gs_graphics_uniform_desc_t){
			.name	= "u_proj",
			.layout = &(gs_graphics_uniform_layout_desc_t){
				.type = GS_GRAPHICS_UNIFORM_MAT4,
			},
			.stage = GS_GRAPHICS_SHADER_STAGE_VERTEX,
		});
	g_renderer->u_view = gs_graphics_uniform_create(
		&(gs_graphics_uniform_desc_t){
			.name	= "u_view",
			.layout = &(gs_graphics_uniform_layout_desc_t){
				.type = GS_GRAPHICS_UNIFORM_MAT4,
			},
			.stage = GS_GRAPHICS_SHADER_STAGE_VERTEX,
		});
	g_renderer->u_light = gs_graphics_uniform_create(
		&(gs_graphics_uniform_desc_t){
			.name	     = "u_light",
			.layout_size = 3 * sizeof(gs_graphics_uniform_layout_desc_t),
			.layout	     = &(gs_graphics_uniform_layout_desc_t[]){
				     {
					     .type  = GS_GRAPHICS_UNIFORM_VEC3,
					     .fname = ".ambient",
				     },
				     {
					     .type  = GS_GRAPHICS_UNIFORM_VEC3,
					     .fname = ".directional",
				     },
				     {
					     .type  = GS_GRAPHICS_UNIFORM_VEC3,
					     .fname = ".direction",
				     },
			     },
			.stage = GS_GRAPHICS_SHADER_STAGE_FRAGMENT,
		});
	g_renderer->u_tex = gs_graphics_uniform_create(
		&(gs_graphics_uniform_desc_t){
			.name	= "u_tex",
			.layout = &(gs_graphics_uniform_layout_desc_t){
				.type = GS_GRAPHICS_UNIFORM_SAMPLER2D,
			},
			.stage = GS_GRAPHICS_SHADER_STAGE_FRAGMENT,
		});
	g_renderer->u_color = gs_graphics_uniform_create(
		&(gs_graphics_uniform_desc_t){
			.name	= "u_color",
			.layout = &(gs_graphics_uniform_layout_desc_t){
				.type = GS_GRAPHICS_UNIFORM_VEC4,
			},
			.stage = GS_GRAPHICS_SHADER_STAGE_FRAGMENT,
		});
	g_renderer->u_barrel_enabled = gs_graphics_uniform_create(
		&(gs_graphics_uniform_desc_t){
			.name	= "u_barrel_enabled",
			.layout = &(gs_graphics_uniform_layout_desc_t){
				.type = GS_GRAPHICS_UNIFORM_INT,
			},
			.stage = GS_GRAPHICS_SHADER_STAGE_VERTEX,
		});
	g_renderer->u_barrel_strength = gs_graphics_uniform_create(
		&(gs_graphics_uniform_desc_t){
			.name	= "u_barrel_strength",
			.layout = &(gs_graphics_uniform_layout_desc_t){
				.type = GS_GRAPHICS_UNIFORM_FLOAT,
			},
			.stage = GS_GRAPHICS_SHADER_STAGE_VERTEX,
		});
	g_renderer->u_barrel_height = gs_graphics_uniform_create(
		&(gs_graphics_uniform_desc_t){
			.name	= "u_barrel_height",
			.layout = &(gs_graphics_uniform_layout_desc_t){
				.type = GS_GRAPHICS_UNIFORM_FLOAT,
			},
			.stage = GS_GRAPHICS_SHADER_STAGE_VERTEX,
		});
	g_renderer->u_barrel_aspect = gs_graphics_uniform_create(
		&(gs_graphics_uniform_desc_t){
			.name	= "u_barrel_aspect",
			.layout = &(gs_graphics_uniform_layout_desc_t){
				.type = GS_GRAPHICS_UNIFORM_FLOAT,
			},
			.stage = GS_GRAPHICS_SHADER_STAGE_VERTEX,
		});
	g_renderer->u_barrel_cyl_ratio = gs_graphics_uniform_create(
		&(gs_graphics_uniform_desc_t){
			.name	= "u_barrel_cyl_ratio",
			.layout = &(gs_graphics_uniform_layout_desc_t){
				.type = GS_GRAPHICS_UNIFORM_FLOAT,
			},
			.stage = GS_GRAPHICS_SHADER_STAGE_VERTEX,
		});

	// Pipeline vertex attributes
	size_t total_stride			     = sizeof(float32_t) * 8;
	gs_graphics_vertex_attribute_desc_t vattrs[] = {
		(gs_graphics_vertex_attribute_desc_t){.format = GS_GRAPHICS_VERTEX_ATTRIBUTE_FLOAT3, .name = "a_pos", .stride = total_stride, .offset = 0},
		(gs_graphics_vertex_attribute_desc_t){.format = GS_GRAPHICS_VERTEX_ATTRIBUTE_FLOAT3, .name = "a_normal", .stride = total_stride, .offset = sizeof(float32_t) * 3},
		(gs_graphics_vertex_attribute_desc_t){.format = GS_GRAPHICS_VERTEX_ATTRIBUTE_FLOAT2, .name = "a_texcoord", .stride = total_stride, .offset = sizeof(float32_t) * 6},
	};
	gs_graphics_vertex_attribute_desc_t post_vattrs[] = {
		(gs_graphics_vertex_attribute_desc_t){.format = GS_GRAPHICS_VERTEX_ATTRIBUTE_FLOAT2, .name = "a_pos", .stride = sizeof(float32_t) * 2, .offset = 0},
	};

	// Create pipelines
	g_renderer->pipe = gs_graphics_pipeline_create(
		&(gs_graphics_pipeline_desc_t){
			.raster = {
				.shader			   = mg_renderer_get_shader("basic"),
				.index_buffer_element_size = sizeof(int32_t),
				.primitive		   = GS_GRAPHICS_PRIMITIVE_TRIANGLES,
			},
			.blend = {
				.func = GS_GRAPHICS_BLEND_EQUATION_ADD,
				.src  = GS_GRAPHICS_BLEND_MODE_SRC_ALPHA,
				.dst  = GS_GRAPHICS_BLEND_MODE_ONE_MINUS_SRC_ALPHA,
			},
			.depth = {
				.func = GS_GRAPHICS_DEPTH_FUNC_LESS,
			},
			.layout = {
				.attrs = vattrs,
				.size  = sizeof(vattrs),
			},
		});
	g_renderer->wire_pipe = gs_graphics_pipeline_create(
		&(gs_graphics_pipeline_desc_t){
			.raster = {
				.shader			   = mg_renderer_get_shader("wireframe"),
				.index_buffer_element_size = sizeof(int32_t),
				.primitive		   = GS_GRAPHICS_PRIMITIVE_LINE_LOOP,
			},
			.blend = {
				.func = GS_GRAPHICS_BLEND_EQUATION_ADD,
				.src  = GS_GRAPHICS_BLEND_MODE_SRC_ALPHA,
				.dst  = GS_GRAPHICS_BLEND_MODE_ONE_MINUS_SRC_ALPHA,
			},
			.depth = {
				.func = GS_GRAPHICS_DEPTH_FUNC_LESS,
			},
			.layout = {
				.attrs = vattrs,
				.size  = sizeof(vattrs),
			},
		});
	g_renderer->post_pipe = gs_graphics_pipeline_create(
		&(gs_graphics_pipeline_desc_t){
			.raster = {
				.shader			   = mg_renderer_get_shader("post"),
				.index_buffer_element_size = sizeof(int32_t),
			},
			.depth = {
				.func = GS_GRAPHICS_DEPTH_FUNC_ALWAYS,
			},
			.layout = {
				.attrs = post_vattrs,
				.size  = sizeof(post_vattrs),
			},
		});

	// Create missing texture
	uint32_t missing_size = 10;
	gs_color_t *pixels    = mg_get_missing_texture_pixels(missing_size);

	g_renderer->missing_texture = gs_graphics_texture_create(
		&(gs_graphics_texture_desc_t){
			.type	    = GS_GRAPHICS_TEXTURE_2D,
			.width	    = missing_size,
			.height	    = missing_size,
			.format	    = GS_GRAPHICS_TEXTURE_FORMAT_RGBA8,
			.min_filter = GS_GRAPHICS_TEXTURE_FILTER_NEAREST,
			.mag_filter = GS_GRAPHICS_TEXTURE_FILTER_NEAREST,
			.mip_filter = GS_GRAPHICS_TEXTURE_FILTER_NEAREST,
			.num_mips   = 0,
			.data	    = pixels});

	gs_free(pixels);
}

void mg_renderer_update()
{
	mg_time_manager_render_start();

	// Framebuffer size
	const gs_vec2 fb = gs_platform_framebuffer_sizev(gs_platform_main_window());
	if (fb.x != g_renderer->fb_size.x || fb.y != g_renderer->fb_size.y)
		_mg_renderer_resize(fb);

	bool32_t has_cam = g_renderer->cam != NULL;

	if (has_cam)
	{
		g_renderer->cam->fov	      = mg_cvar("r_fov")->value.i;
		g_renderer->cam->aspect_ratio = g_renderer->fb_size.x / g_renderer->fb_size.y;
		g_renderer->offscreen_cleared = false;

		// Render bsp to offscreen texture
		if (g_game_manager->map != NULL && g_game_manager->map->valid)
		{
			bsp_map_update(g_game_manager->map, g_renderer->cam, g_renderer->fb_size);
			bsp_map_render(g_game_manager->map, g_renderer->cam, g_renderer->offscreen_rp, &g_renderer->cb, g_renderer->fb_size);
			g_renderer->offscreen_cleared = true;
		}

		// Render models to offscreen texture
		_mg_renderer_renderable_pass();

		// Post-process offscreen texture and render to backbuffer.
		_mg_renderer_post_pass();
	}
	else
	{
		mg_time_manager_models_start();
		mg_time_manager_models_end();
		mg_time_manager_post_start();
		mg_time_manager_post_end();
	}

	// Render UI straight to backbuffer
	mg_ui_manager_render(g_renderer->fb_size, !has_cam);

	// Submit command buffer
	mg_time_manager_submit_start();
	gs_graphics_command_buffer_submit(&g_renderer->cb);
	mg_time_manager_submit_end();

	mg_time_manager_render_end();
}

void mg_renderer_free()
{
	for (size_t i = 0; i < gs_dyn_array_size(g_renderer->shader_names); i++)
	{
		gs_free(g_renderer->shader_names[i]);
		g_renderer->shader_names[i] = NULL;
		gs_graphics_shader_destroy(g_renderer->shaders[i]);
		gs_free(g_renderer->shader_sources_frag[i]);
		gs_free(g_renderer->shader_sources_vert[i]);
	}

	gs_dyn_array_free(g_renderer->shader_names);
	gs_dyn_array_free(g_renderer->shaders);
	gs_dyn_array_free(g_renderer->shader_sources_frag);
	gs_dyn_array_free(g_renderer->shader_sources_vert);

	gs_slot_array_free(g_renderer->renderables);

	gs_graphics_uniform_destroy(g_renderer->u_proj);
	gs_graphics_uniform_destroy(g_renderer->u_view);
	gs_graphics_uniform_destroy(g_renderer->u_light);
	gs_graphics_uniform_destroy(g_renderer->u_tex);
	gs_graphics_uniform_destroy(g_renderer->u_color);
	gs_graphics_uniform_destroy(g_renderer->u_barrel_enabled);
	gs_graphics_uniform_destroy(g_renderer->u_barrel_strength);
	gs_graphics_uniform_destroy(g_renderer->u_barrel_height);
	gs_graphics_uniform_destroy(g_renderer->u_barrel_aspect);
	gs_graphics_uniform_destroy(g_renderer->u_barrel_cyl_ratio);

	gs_graphics_renderpass_destroy(g_renderer->offscreen_rp);

	gs_graphics_framebuffer_destroy(g_renderer->offscreen_fbo);

	gs_graphics_texture_destroy(g_renderer->missing_texture);
	gs_graphics_texture_destroy(g_renderer->offscreen_rt);
	gs_graphics_texture_destroy(g_renderer->offscreen_dt);

	gs_command_buffer_free(&g_renderer->cb);
	gs_immediate_draw_free(&g_renderer->gsi);
	// TODO: free other things in gui
	// gs_gui_free(&g_renderer->gui);
	gs_immediate_draw_free(&g_renderer->gui.gsi);
	gs_graphics_pipeline_destroy(g_renderer->pipe);
	gs_graphics_pipeline_destroy(g_renderer->wire_pipe);
	gs_graphics_pipeline_destroy(g_renderer->post_pipe);

	gs_graphics_index_buffer_destroy(g_renderer->screen_ibo);
	gs_free(g_renderer->screen_indices);

	gs_graphics_vertex_buffer_destroy(g_renderer->screen_vbo);
	gs_free(g_renderer->screen_vertices);

	gs_free(g_renderer);
	g_renderer = NULL;
}

uint32_t mg_renderer_create_renderable(mg_model_t model, gs_vqs *transform)
{
	mg_renderable_t renderable = {
		.model		   = model,
		.transform	   = transform,
		.u_view		   = gs_vqs_to_mat4(transform),
		.frame		   = 0,
		.prev_frame_time   = g_time_manager->time,
		.current_animation = NULL,
	};

	return gs_slot_array_insert(g_renderer->renderables, renderable);
}

void mg_renderer_remove_renderable(uint32_t renderable_id)
{
	gs_slot_array_erase(g_renderer->renderables, renderable_id);
}

mg_renderable_t *mg_renderer_get_renderable(uint32_t renderable_id)
{
	if (renderable_id >= gs_slot_array_size(g_renderer->renderables))
	{
		mg_println("WARN: invalid renderable_id %zu in mg_renderer_get_renderable", renderable_id);
		return NULL;
	}

	return gs_slot_array_getp(g_renderer->renderables, renderable_id);
}

gs_handle(gs_graphics_shader_t) mg_renderer_get_shader(char *name)
{
	for (size_t i = 0; i < gs_dyn_array_size(g_renderer->shader_names); i++)
	{
		if (strcmp(name, g_renderer->shader_names[i]) == 0)
		{
			return g_renderer->shaders[i];
		}
	}

	mg_println("ERR: mg_renderer_get_shader no shader %s", name);

	return gs_handle_invalid(gs_graphics_shader_t);
}

bool32_t mg_renderer_play_animation(uint32_t id, char *name)
{
	mg_renderable_t *renderable = mg_renderer_get_renderable(id);
	if (renderable == NULL)
	{
		return false;
	}

	bool32_t found = false;

	for (size_t i = 0; i < gs_dyn_array_size(renderable->model.data->animations); i++)
	{
		if (strcmp(renderable->model.data->animations[i].name, name) == 0)
		{
			renderable->prev_frame_time   = g_time_manager->time;
			renderable->current_animation = &renderable->model.data->animations[i];
			renderable->frame	      = renderable->current_animation->first_frame;
			found			      = true;
			break;
		}
	}

	if (!found)
	{
		mg_println("WARN: mg_model_manager_play_animation could not find animation %s in model %s", name, renderable->model.filename);
	}

	return found;
}

void _mg_renderer_resize(const gs_vec2 fb_size)
{
	g_renderer->fb_size = fb_size;

	if (gs_handle_is_valid(g_renderer->offscreen_rt))
	{
		gs_graphics_texture_destroy(g_renderer->offscreen_rt);
		g_renderer->offscreen_rt = gs_handle_invalid(gs_graphics_texture_t);
	}

	if (gs_handle_is_valid(g_renderer->offscreen_dt))
	{
		gs_graphics_texture_destroy(g_renderer->offscreen_dt);
		g_renderer->offscreen_dt = gs_handle_invalid(gs_graphics_texture_t);
	}

	g_renderer->offscreen_rt = gs_graphics_texture_create(
		&(gs_graphics_texture_desc_t){
			.type	    = GS_GRAPHICS_TEXTURE_2D,
			.width	    = fb_size.x,
			.height	    = fb_size.y,
			.format	    = GS_GRAPHICS_TEXTURE_FORMAT_RGBA8,
			.wrap_s	    = GS_GRAPHICS_TEXTURE_WRAP_CLAMP_TO_BORDER,
			.wrap_t	    = GS_GRAPHICS_TEXTURE_WRAP_CLAMP_TO_BORDER,
			.min_filter = GS_GRAPHICS_TEXTURE_FILTER_NEAREST,
			.mag_filter = GS_GRAPHICS_TEXTURE_FILTER_NEAREST,
			.mip_filter = GS_GRAPHICS_TEXTURE_FILTER_NEAREST,
			.num_mips   = 0,
		});

	g_renderer->offscreen_dt = gs_graphics_texture_create(
		&(gs_graphics_texture_desc_t){
			.type	    = GS_GRAPHICS_TEXTURE_2D,
			.width	    = fb_size.x,
			.height	    = fb_size.y,
			.format	    = GS_GRAPHICS_TEXTURE_FORMAT_DEPTH32F,
			.wrap_s	    = GS_GRAPHICS_TEXTURE_WRAP_CLAMP_TO_BORDER,
			.wrap_t	    = GS_GRAPHICS_TEXTURE_WRAP_CLAMP_TO_BORDER,
			.min_filter = GS_GRAPHICS_TEXTURE_FILTER_NEAREST,
			.mag_filter = GS_GRAPHICS_TEXTURE_FILTER_NEAREST,
			.mip_filter = GS_GRAPHICS_TEXTURE_FILTER_NEAREST,
			.num_mips   = 0,
		});
}

void _mg_renderer_renderable_pass()
{
	mg_time_manager_models_start();

	if (gs_slot_array_size(g_renderer->renderables) == 0)
	{
		mg_time_manager_models_end();
		return;
	}

	bool wireframe = mg_cvar("r_wireframe")->value.i;

	gs_renderpass renderables_pass = gs_default_val();

	// Uniforms that don't change per renderable
	gs_mat4 u_proj = mg_camera_get_view_projection(g_renderer->cam, (s32)g_renderer->fb_size.x, (s32)g_renderer->fb_size.y);

	// Uniform binds
	gs_graphics_bind_uniform_desc_t uniforms[] = {
		{
			.uniform = g_renderer->u_proj,
			.data	 = &u_proj,
			.binding = 0, // VERTEX
		},
		{0}, // u_view, VERTEX
		{0}, // u_light or u_color FRAGMENT
		{0}, // u_tex, FRAGMENT
	};
	uint8_t uniform_count = wireframe ? 3 : 4;

	// Begin render
	gs_graphics_renderpass_begin(&g_renderer->cb, g_renderer->offscreen_rp);
	gs_graphics_set_viewport(&g_renderer->cb, 0, 0, (int32_t)g_renderer->fb_size.x, (int32_t)g_renderer->fb_size.y);
	gs_graphics_pipeline_bind(&g_renderer->cb, wireframe ? g_renderer->wire_pipe : g_renderer->pipe);

	if (!g_renderer->offscreen_cleared)
	{
		// Didn't clear in bsp pass
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

	// Draw all renderables
	for (
		gs_slot_array_iter it = gs_slot_array_iter_new(g_renderer->renderables);
		gs_slot_array_iter_valid(g_renderer->renderables, it);
		gs_slot_array_iter_advance(g_renderer->renderables, it))
	{
		mg_renderable_t *renderable = gs_slot_array_iter_getp(g_renderer->renderables, it);

		// View matrix
		renderable->u_view = gs_vqs_to_mat4(renderable->transform);
		uniforms[1]	   = (gs_graphics_bind_uniform_desc_t){
			       .uniform = g_renderer->u_view,
			       .data	= &renderable->u_view,
			       .binding = 1, // VERTEX
		       };

		gs_vec4_t color		  = gs_v4(1.0, 1.0, 1.0, 1.0);
		mg_renderer_light_t light = {0};

		if (wireframe)
		{
			// Color
			uniforms[2] = (gs_graphics_bind_uniform_desc_t){
				.uniform = g_renderer->u_color,
				.data	 = &color,
				.binding = 0, // FRAGMENT
			};
		}
		else
		{
			// Light
			if (g_game_manager->map != NULL && g_game_manager->map->valid)
			{
				light = bsp_sample_lightvol(g_game_manager->map, renderable->transform->position);
				// bsp_lightvol_lump_t lump = bsp_get_lightvol(g_game_manager->map, renderable->transform->position, NULL);
				// light.directional.x	 = (float)lump.directional[0] / 255.0f;
				// light.directional.y	 = (float)lump.directional[1] / 255.0f;
				// light.directional.z	 = (float)lump.directional[2] / 255.0f;
				// light.ambient.x		 = (float)lump.ambient[0] / 255.0f;
				// light.ambient.y		 = (float)lump.ambient[1] / 255.0f;
				// light.ambient.z		 = (float)lump.ambient[2] / 255.0f;
				// light.direction		 = mg_sphere_to_normal(lump.dir);
			}
			else
			{
				light = (mg_renderer_light_t){
					.ambient     = gs_v3(0.4f, 0.4f, 0.4f),
					.directional = gs_v3(0.8f, 0.8f, 0.8f),
					.direction   = gs_vec3_norm(gs_v3(0.3f, 0.5f, -0.5f)),
				};
			}
			uniforms[2] = (gs_graphics_bind_uniform_desc_t){
				.uniform = g_renderer->u_light,
				.data	 = &light,
				.binding = 0, // FRAGMENT
			};
		}

		// Draw each surface
		for (size_t i = 0; i < renderable->model.data->header.num_surfaces; i++)
		{
			md3_surface_t surf = renderable->model.data->surfaces[i];

			if (!wireframe)
			{
				// Texture
				uniforms[3] = (gs_graphics_bind_uniform_desc_t){
					.uniform = g_renderer->u_tex,
					.data	 = ((surf.textures[0] != NULL && gs_handle_is_valid(surf.textures[0]->hndl)) ? &surf.textures[0]->hndl : &g_renderer->missing_texture),
					.binding = 1, // FRAGMENT
				};
			}

			// Construct binds
			gs_graphics_bind_desc_t binds = {
				.vertex_buffers = {
					.desc = &(gs_graphics_bind_vertex_buffer_desc_t){
						.buffer = surf.vbos[renderable->frame],
					},
				},
				.index_buffers = {
					.desc = &(gs_graphics_bind_index_buffer_desc_t){
						.buffer = surf.ibo,
					},
				},
				.uniforms = {
					.desc = uniforms,
					.size = sizeof(gs_graphics_bind_uniform_desc_t) * uniform_count,
				},
			};

			gs_graphics_apply_bindings(&g_renderer->cb, &binds);
			gs_graphics_draw(&g_renderer->cb, &(gs_graphics_draw_desc_t){.start = 0, .count = surf.num_tris * 3});
		}

		// Play animation
		if (renderable->current_animation != NULL)
		{
			double plat_time	= g_time_manager->time;
			double frame_time	= 1.0f / renderable->current_animation->fps;
			double since_last_frame = plat_time - renderable->prev_frame_time;

			if (since_last_frame >= frame_time)
			{
				renderable->frame++;

				if (since_last_frame >= frame_time * 10)
				{
					// Don't fast-forward when missing updates.
					// Game frozen, paused at breakpoint, etc...
					renderable->prev_frame_time = plat_time;
				}
				else
				{
					renderable->prev_frame_time += frame_time;
				}

				if (renderable->frame >= renderable->current_animation->first_frame + renderable->current_animation->num_frames)
				{
					if (renderable->current_animation->loop)
					{
						// Reset to first frame
						renderable->frame = renderable->current_animation->first_frame;
					}
					else
					{
						// Freeze at final frame
						renderable->frame = renderable->current_animation->first_frame + renderable->current_animation->num_frames - 1;
					}
				}

				// Sanity
				if (renderable->frame >= renderable->model.data->header.num_frames)
				{
					mg_println(
						"ERR: _mg_renderer_renderable_pass animation '%s' exceeds model '%s' num_frames %d",
						renderable->current_animation->name,
						renderable->model.filename,
						renderable->model.data->header.num_frames);
					renderable->frame	      = 0;
					renderable->current_animation = NULL;
				}
			}
		}
	}

	gs_graphics_renderpass_end(&g_renderer->cb);

	mg_time_manager_models_end();
}

void _mg_renderer_post_pass()
{
	mg_time_manager_post_start();

	gs_graphics_renderpass_begin(&g_renderer->cb, GS_GRAPHICS_RENDER_PASS_DEFAULT);
	gs_graphics_set_viewport(&g_renderer->cb, 0, 0, (int32_t)g_renderer->fb_size.x, (int32_t)g_renderer->fb_size.y);
	gs_graphics_pipeline_bind(&g_renderer->cb, g_renderer->post_pipe);

	float32_t barrel_height = tanf(0.5 * gs_deg2rad(mg_cvar("r_fov")->value.i / g_renderer->cam->aspect_ratio));

	// Uniform binds
	gs_graphics_bind_uniform_desc_t uniforms[] = {
		(gs_graphics_bind_uniform_desc_t){
			.uniform = g_renderer->u_tex,
			.data	 = &g_renderer->offscreen_rt,
			.binding = 0, // FRAGMENT
		},
		(gs_graphics_bind_uniform_desc_t){
			.uniform = g_renderer->u_barrel_enabled,
			.data	 = &mg_cvar("r_barrel_enabled")->value.i,
			.binding = 1, // VERTEX
		},
		(gs_graphics_bind_uniform_desc_t){
			.uniform = g_renderer->u_barrel_strength,
			.data	 = &mg_cvar("r_barrel_strength")->value.f,
			.binding = 1, // VERTEX
		},
		(gs_graphics_bind_uniform_desc_t){
			.uniform = g_renderer->u_barrel_height,
			.data	 = &barrel_height,
			.binding = 2, // VERTEX
		},
		(gs_graphics_bind_uniform_desc_t){
			.uniform = g_renderer->u_barrel_aspect,
			.data	 = &g_renderer->cam->aspect_ratio,
			.binding = 3, // VERTEX
		},
		(gs_graphics_bind_uniform_desc_t){
			.uniform = g_renderer->u_barrel_cyl_ratio,
			.data	 = &mg_cvar("r_barrel_cyl_ratio")->value.f,
			.binding = 4, // VERTEX
		},
	};

	// Construct binds
	gs_graphics_bind_desc_t binds = {
		.vertex_buffers = {
			.desc = &(gs_graphics_bind_vertex_buffer_desc_t){
				.buffer = g_renderer->screen_vbo,
			},
		},
		.index_buffers = {
			.desc = &(gs_graphics_bind_index_buffer_desc_t){
				.buffer = g_renderer->screen_ibo,
			},
		},
		.uniforms = {
			.desc = uniforms,
			.size = sizeof(uniforms),
		},
	};

	gs_graphics_apply_bindings(&g_renderer->cb, &binds);
	gs_graphics_draw(&g_renderer->cb, &(gs_graphics_draw_desc_t){.start = 0, .count = 6});
	gs_graphics_renderpass_end(&g_renderer->cb);

	mg_time_manager_post_end();
}

void _mg_renderer_load_shader(char *name)
{
	// Get paths to shaders
	char *base_path = "assets/shaders/";

	char *vert_ext = "_vs.glsl";
	char *frag_ext = "_fs.glsl";

	size_t sz  = strlen(base_path) + strlen(name) + strlen(vert_ext) + 1;
	char *vert = gs_malloc(sz);
	memset(vert, 0, sz);
	strcat(vert, base_path);
	strcat(vert, name);
	strcat(vert, vert_ext);

	sz	   = strlen(base_path) + strlen(name) + strlen(frag_ext) + 1;
	char *frag = gs_malloc(sz);
	memset(frag, 0, sz);
	strcat(frag, base_path);
	strcat(frag, name);
	strcat(frag, frag_ext);

	// Sanity check
	if (!gs_platform_file_exists(vert))
	{
		mg_println("ERR: _mg_renderer_load_shader no shader %s", vert);
		gs_free(vert);
		gs_free(frag);
		return;
	}
	if (!gs_platform_file_exists(frag))
	{
		mg_println("ERR: _mg_renderer_load_shader no shader %s", frag);
		gs_free(vert);
		gs_free(frag);
		return;
	}

	// Read from files
	char *vert_src = gs_platform_read_file_contents(vert, "r", &sz);
	mg_println("_mg_renderer_load_shader read %zu bytes from %s", sz, vert);
	gs_dyn_array_push(g_renderer->shader_sources_vert, vert_src);

	char *frag_src = gs_platform_read_file_contents(frag, "r", &sz);
	mg_println("_mg_renderer_load_shader read %zu bytes from %s", sz, frag);
	gs_dyn_array_push(g_renderer->shader_sources_frag, frag_src);

	// Create description
	gs_graphics_shader_source_desc_t sources[] = {
		(gs_graphics_shader_source_desc_t){.type = GS_GRAPHICS_SHADER_STAGE_VERTEX, .source = vert_src},
		(gs_graphics_shader_source_desc_t){.type = GS_GRAPHICS_SHADER_STAGE_FRAGMENT, .source = frag_src},
	};

	// Create shader
	gs_handle_gs_graphics_shader_t shader = gs_graphics_shader_create(
		&(gs_graphics_shader_desc_t){
			.sources = sources,
			.size	 = sizeof(sources),
			.name	 = name,
		});

	gs_dyn_array_push(g_renderer->shaders, shader);
	// make a persistent copy
	char *name_cpy = gs_malloc(strlen(name) + 1);
	memcpy(name_cpy, name, strlen(name) + 1);
	gs_dyn_array_push(g_renderer->shader_names, name_cpy);

	gs_free(vert);
	gs_free(frag);
}