/*================================================================
	* graphics/renderer.c
	*
	* Copyright (c) 2021 Lauri Räsänen
	* ================================

	...
=================================================================*/

#include "renderer.h"
#include "../util/camera.h"
#include "../util/render.h"

mg_renderer_t *g_renderer;

void mg_renderer_init()
{
	// Allocate
	g_renderer		 = gs_malloc_init(mg_renderer_t);
	g_renderer->cb		 = gs_command_buffer_new();
	g_renderer->gsi		 = gs_immediate_draw_new();
	g_renderer->renderables	 = gs_slot_array_new(mg_renderable_t);
	g_renderer->shaders	 = gs_dyn_array_new(gs_handle_gs_graphics_shader_t);
	g_renderer->shader_names = gs_dyn_array_new(char *);

	_mg_renderer_load_shader("basic");
	_mg_renderer_load_shader("basic_unlit");
	_mg_renderer_load_shader("bsp");

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

	// Pipeline vertex attributes
	gs_graphics_vertex_attribute_desc_t vattrs[] = {
		(gs_graphics_vertex_attribute_desc_t){.format = GS_GRAPHICS_VERTEX_ATTRIBUTE_FLOAT3, .name = "a_pos"},
		(gs_graphics_vertex_attribute_desc_t){.format = GS_GRAPHICS_VERTEX_ATTRIBUTE_FLOAT3, .name = "a_normal"},
		(gs_graphics_vertex_attribute_desc_t){.format = GS_GRAPHICS_VERTEX_ATTRIBUTE_FLOAT2, .name = "a_texcoord"},
	};

	// Create pipeline
	g_renderer->pipe = gs_graphics_pipeline_create(
		&(gs_graphics_pipeline_desc_t){
			.raster = {
				.shader			   = mg_renderer_get_shader("basic"),
				.index_buffer_element_size = sizeof(int32_t),
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

	// Create missing texture
	uint32_t missing_size = 10;
	gs_color_t *pixels    = mg_get_missing_texture_pixels(missing_size);

	g_renderer->missing_texture = gs_graphics_texture_create(
		&(gs_graphics_texture_desc_t){
			.width	    = missing_size,
			.height	    = missing_size,
			.format	    = GS_GRAPHICS_TEXTURE_FORMAT_RGBA8,
			.min_filter = GS_GRAPHICS_TEXTURE_FILTER_NEAREST,
			.mag_filter = GS_GRAPHICS_TEXTURE_FILTER_NEAREST,
			.data	    = pixels});

	gs_free(pixels);
}

void mg_renderer_update()
{
	// Framebuffer size
	const gs_vec2 fb = gs_platform_framebuffer_sizev(gs_platform_main_window());

	// Render passes
	if (g_renderer->bsp != NULL && g_renderer->bsp->valid)
	{
		bsp_map_update(g_renderer->bsp, g_renderer->cam->transform.position);
		bsp_map_render(g_renderer->bsp, g_renderer->cam);
	}
	_mg_renderer_renderable_pass(fb);
	_mg_renderer_immediate_pass(fb);

	// Submit command buffer (syncs to GPU, MUST be done on main thread where you have your GPU context created)
	gs_graphics_submit_command_buffer(&g_renderer->cb);
}

void mg_renderer_free()
{
	for (size_t i = 0; i < gs_dyn_array_size(g_renderer->shader_names); i++)
	{
		gs_free(g_renderer->shader_names[i]);
		g_renderer->shader_names[i] = NULL;
	}

	gs_immediate_draw_free(&g_renderer->gsi);
	gs_command_buffer_free(&g_renderer->cb);

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
		.prev_frame_time   = gs_platform_elapsed_time(),
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
		gs_println("WARN: invalid renderable_id %zu in mg_renderer_get_renderable", renderable_id);
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

	gs_println("ERR: mg_renderer_get_shader no shader %s", name);

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
			renderable->current_animation = &renderable->model.data->animations[i];
			renderable->frame	      = renderable->current_animation->first_frame;
			found			      = true;
			break;
		}
	}

	if (!found)
	{
		gs_println("WARN: mg_model_manager_play_animation could not find animation %s in model %s", name, renderable->model.filename);
	}

	return found;
}

void _mg_renderer_renderable_pass(gs_vec2 fb)
{
	if (g_renderer->cam == NULL) return;
	if (gs_slot_array_size(g_renderer->renderables) == 0) return;

	gs_renderpass renderables_pass = gs_default_val();

	// Uniforms that don't chang per renderable
	gs_mat4 u_proj = mg_camera_get_view_projection(g_renderer->cam, (s32)fb.x, (s32)fb.y);

	// Uniform binds
	gs_graphics_bind_uniform_desc_t uniforms[] = {
		{
			.uniform = g_renderer->u_proj,
			.data	 = &u_proj,
			.binding = 0, // VERTEX
		},
		{0}, // u_view, VERTEX
		{0}, // u_light, FRAGMENT
		{0}, // u_tex, FRAGMENT
	};

	// Begin render
	gs_graphics_begin_render_pass(&g_renderer->cb, GS_GRAPHICS_RENDER_PASS_DEFAULT);
	gs_graphics_set_viewport(&g_renderer->cb, 0, 0, (int32_t)fb.x, (int32_t)fb.y);
	gs_graphics_bind_pipeline(&g_renderer->cb, g_renderer->pipe);

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

		// Light
		mg_renderer_light_t light = bsp_sample_lightvol(g_renderer->bsp, renderable->transform->position);
		uniforms[2]		  = (gs_graphics_bind_uniform_desc_t){
			      .uniform = g_renderer->u_light,
			      .data    = &light,
			      .binding = 0, // FRAGMENT
		      };

		// Draw each surface
		for (size_t i = 0; i < renderable->model.data->header.num_surfaces; i++)
		{
			md3_surface_t surf = renderable->model.data->surfaces[i];

			// Texture
			uniforms[3] = (gs_graphics_bind_uniform_desc_t){
				.uniform = g_renderer->u_tex,
				.data	 = ((surf.textures[0] != NULL && gs_handle_is_valid(surf.textures[0]->hndl)) ? &surf.textures[0]->hndl : &g_renderer->missing_texture),
				.binding = 1, // FRAGMENT
			};

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
					.size = sizeof(uniforms),
				},
			};

			gs_graphics_apply_bindings(&g_renderer->cb, &binds);
			gs_graphics_draw(&g_renderer->cb, &(gs_graphics_draw_desc_t){.start = 0, .count = surf.num_tris * 3});
		}

		// Play animation
		if (renderable->current_animation != NULL)
		{
			double plat_time  = gs_platform_elapsed_time();
			double frame_time = 1000 / renderable->current_animation->fps;

			if (plat_time - renderable->prev_frame_time >= frame_time)
			{
				renderable->frame++;
				renderable->prev_frame_time += frame_time;

				if (renderable->frame >= renderable->current_animation->first_frame + renderable->current_animation->num_frames)
				{
					if (renderable->current_animation->loop)
					{
						renderable->frame = renderable->current_animation->first_frame;
					}
					else
					{
						renderable->current_animation = NULL;
					}
				}

				// Sanity
				if (renderable->frame >= renderable->model.data->header.num_frames)
				{
					gs_println(
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

	gs_graphics_end_render_pass(&g_renderer->cb);
}

void _mg_renderer_immediate_pass(gs_vec2 fb)
{
	if (!g_renderer->use_immediate_mode) return;

	_mg_renderer_draw_debug_overlay();
	gs_renderpass im_pass = gs_default_val();
	gs_graphics_begin_render_pass(&g_renderer->cb, im_pass);
	gs_graphics_set_viewport(&g_renderer->cb, 0, 0, (int32_t)fb.x, (int32_t)fb.y);
	gsi_draw(&g_renderer->gsi, &g_renderer->cb);
	gs_graphics_end_render_pass(&g_renderer->cb);
}

void _mg_renderer_draw_debug_overlay()
{
	// draw fps
	char temp[64];
	sprintf(temp, "fps: %d", (int)gs_round(1.0f / gs_platform_delta_time()));
	gsi_camera2D(&g_renderer->gsi);
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

void _mg_renderer_load_shader(char *name)
{
	// Get paths to shaders
	char *base_path = "shaders/";
	char *vert_ext	= "_vs.glsl";
	char *frag_ext	= "_fs.glsl";

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
	if (!gs_util_file_exists(vert))
	{
		gs_println("ERR: _mg_renderer_load_shader no shader %s", vert);
		gs_free(vert);
		gs_free(frag);
		return;
	}
	if (!gs_util_file_exists(frag))
	{
		gs_println("ERR: _mg_renderer_load_shader no shader %s", frag);
		gs_free(vert);
		gs_free(frag);
		return;
	}

	// Read from files
	char *vert_src = gs_platform_read_file_contents(vert, "r", &sz);
	gs_println("_mg_renderer_load_shader read %zu bytes from %s", sz, vert);

	char *frag_src = gs_platform_read_file_contents(frag, "r", &sz);
	gs_println("_mg_renderer_load_shader read %zu bytes from %s", sz, frag);

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