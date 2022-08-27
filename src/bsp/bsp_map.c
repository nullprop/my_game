/*================================================================
	* bsp/bsp_map.c
	*
	* Copyright (c) 2021 Lauri Räsänen
	* Copyright (c) 2018 Krzysztof Kondrak
	*
	* See README.md for license.
	* ================================

	BSP rendering.
=================================================================*/

#include "bsp_map.h"
#include "../game/config.h"
#include "../game/time_manager.h"
#include "../graphics/renderer.h"
#include "../graphics/texture_manager.h"
#include "../util/camera.h"
#include "../util/render.h"
#include "../util/transform.h"

void bsp_map_init(bsp_map_t *map)
{
	map->previous_leaf = uint32_max;

	// Init dynamic arrays
	gs_dyn_array_reserve(map->render_faces, map->faces.count);
	uint32_t patch_count = 0;
	for (size_t i = 0; i < map->faces.count; i++)
	{
		if (map->faces.data[i].type == BSP_FACE_TYPE_PATCH)
		{
			patch_count++;
		}
	}
	gs_dyn_array_reserve(map->patches, patch_count);

	// Load stuff
	_bsp_load_entities(map);
	_bsp_load_textures(map);
	_bsp_load_lightmaps(map);
	_bsp_load_lightvols(map);

	uint32_t face_array_idx	 = 0;
	uint32_t patch_array_idx = 0;

	// Create renderable faces and patches
	for (size_t i = 0; i < map->faces.count; i++)
	{
		bsp_face_renderable_t face = {
			.type = map->faces.data[i].type,
		};

		if (face.type == BSP_FACE_TYPE_PATCH)
		{
			_bsp_create_patch(map, map->faces.data[i]);
			// index to map->patches
			face.index = patch_array_idx;
			patch_array_idx++;
		}
		else
		{
			// index to map->faces
			face.index = i;
			face_array_idx++;
		}

		gs_dyn_array_push(map->render_faces, face);
	}

	// Index & Vertex buffers
	_bsp_map_create_buffers(map);

	// Create uniforms
	map->bsp_graphics_u_proj = gs_graphics_uniform_create(
		&(gs_graphics_uniform_desc_t){
			.name	= "u_proj",
			.layout = &(gs_graphics_uniform_layout_desc_t){
				.type = GS_GRAPHICS_UNIFORM_MAT4,
			},
			.stage = GS_GRAPHICS_SHADER_STAGE_VERTEX,
		});
	map->bsp_graphics_u_tex = gs_graphics_uniform_create(
		&(gs_graphics_uniform_desc_t){
			.name	= "u_tex",
			.layout = &(gs_graphics_uniform_layout_desc_t){
				.type = GS_GRAPHICS_UNIFORM_SAMPLER2D,
			},
			.stage = GS_GRAPHICS_SHADER_STAGE_FRAGMENT,
		});
	map->bsp_graphics_u_lm = gs_graphics_uniform_create(
		&(gs_graphics_uniform_desc_t){
			.name	= "u_lm",
			.layout = &(gs_graphics_uniform_layout_desc_t){
				.type = GS_GRAPHICS_UNIFORM_SAMPLER2D,
			},
			.stage = GS_GRAPHICS_SHADER_STAGE_FRAGMENT,
		});
	map->bsp_graphics_u_color = gs_graphics_uniform_create(
		&(gs_graphics_uniform_desc_t){
			.name	= "u_color",
			.layout = &(gs_graphics_uniform_layout_desc_t){
				.type = GS_GRAPHICS_UNIFORM_VEC4,
			},
			.stage = GS_GRAPHICS_SHADER_STAGE_FRAGMENT,
		});

	// Pipeline vertex attributes
	size_t total_stride			     = sizeof(float32_t) * 10 + sizeof(uint8_t) * 4;
	gs_graphics_vertex_attribute_desc_t vattrs[] = {
		(gs_graphics_vertex_attribute_desc_t){.format = GS_GRAPHICS_VERTEX_ATTRIBUTE_FLOAT3, .name = "a_pos", .stride = total_stride, .offset = 0},
		(gs_graphics_vertex_attribute_desc_t){.format = GS_GRAPHICS_VERTEX_ATTRIBUTE_FLOAT2, .name = "a_tex_coord", .stride = total_stride, .offset = sizeof(float32_t) * 3},
		(gs_graphics_vertex_attribute_desc_t){.format = GS_GRAPHICS_VERTEX_ATTRIBUTE_FLOAT2, .name = "a_lm_coord", .stride = total_stride, .offset = sizeof(float32_t) * 5},
		(gs_graphics_vertex_attribute_desc_t){.format = GS_GRAPHICS_VERTEX_ATTRIBUTE_FLOAT3, .name = "a_normal", .stride = total_stride, .offset = sizeof(float32_t) * 7},
		(gs_graphics_vertex_attribute_desc_t){.format = GS_GRAPHICS_VERTEX_ATTRIBUTE_BYTE4, .name = "a_color", .stride = total_stride, .offset = sizeof(float32_t) * 10},
	};

	map->bsp_graphics_pipe = gs_graphics_pipeline_create(
		&(gs_graphics_pipeline_desc_t){
			.raster = {
				.shader			   = mg_renderer_get_shader("bsp"),
				.index_buffer_element_size = sizeof(uint32_t),
				.primitive		   = GS_GRAPHICS_PRIMITIVE_TRIANGLES,
				.face_culling		   = GS_GRAPHICS_FACE_CULLING_BACK,
				.winding_order		   = GS_GRAPHICS_WINDING_ORDER_CW,
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
	map->bsp_graphics_wire_pipe = gs_graphics_pipeline_create(
		&(gs_graphics_pipeline_desc_t){
			.raster = {
				.shader			   = mg_renderer_get_shader("bsp_wireframe"),
				.index_buffer_element_size = sizeof(uint32_t),
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

	// Static stats
	map->stats.total_vertices = gs_dyn_array_size(map->bsp_graphics_vert_arr); // inaccurate, has patch control verts
	map->stats.total_indices  = gs_dyn_array_size(map->bsp_graphics_index_arr);
	map->stats.total_faces	  = face_array_idx;
	map->stats.total_patches  = patch_array_idx;
}

void _bsp_load_entities(bsp_map_t *map)
{
	map->entities = gs_dyn_array_new(bsp_entity_t);

	uint32_t sz = gs_string_length(map->entity_lump.ents) + 1;
	char *ents  = gs_malloc(sz);
	memcpy(ents, map->entity_lump.ents, sz);

	char *ent = strtok(ents, "{");
	while (ent)
	{
		if (gs_string_length(ent) < 1) continue;

		gs_dyn_array_push(map->entities, bsp_entity_from_string(ent));

		ent = strtok(0, "{");
	}

	gs_free(ents);
}

void _bsp_load_textures(bsp_map_t *map)
{
	int32_t num_textures = map->header.dir_entries[BSP_LUMP_TYPE_TEXTURES].length / sizeof(bsp_texture_lump_t);

	map->stats.total_textures = num_textures;
	map->texture_assets.count = num_textures;
	map->texture_assets.data  = gs_malloc(sizeof(gs_asset_texture_t *) * num_textures);

	for (size_t i = 0; i < num_textures; i++)
	{
		map->texture_assets.data[i] = mg_texture_manager_get(map->textures.data[i].name);
		if (map->texture_assets.data[i] != NULL)
		{
			map->stats.loaded_textures++;
		}
	}

	// Create missing texture
	uint32_t missing_size = 10;
	gs_color_t *pixels    = mg_get_missing_texture_pixels(missing_size);

	map->missing_texture = gs_graphics_texture_create(
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

void _bsp_load_lightmaps(bsp_map_t *map)
{
	gs_color_t gray		= gs_color(100, 100, 100, 255);
	map->missing_lm_texture = gs_graphics_texture_create(
		&(gs_graphics_texture_desc_t){
			.type	    = GS_GRAPHICS_TEXTURE_2D,
			.width	    = 1,
			.height	    = 1,
			.format	    = GS_GRAPHICS_TEXTURE_FORMAT_RGBA8,
			.min_filter = GS_GRAPHICS_TEXTURE_FILTER_LINEAR,
			.mag_filter = GS_GRAPHICS_TEXTURE_FILTER_LINEAR,
			.mip_filter = GS_GRAPHICS_TEXTURE_FILTER_LINEAR,
			.num_mips   = 1,
			.data	    = &gray});

	map->lightmap_textures.data  = gs_malloc(map->lightmaps.count * sizeof(gs_handle(gs_graphics_texture_t)));
	map->lightmap_textures.count = map->lightmaps.count;

	for (size_t i = 0; i < map->lightmaps.count; i++)
	{
		map->lightmap_textures.data[i] = gs_graphics_texture_create(
			&(gs_graphics_texture_desc_t){
				.type	    = GS_GRAPHICS_TEXTURE_2D,
				.width	    = 128,
				.height	    = 128,
				.format	    = GS_GRAPHICS_TEXTURE_FORMAT_RGB8,
				.min_filter = GS_GRAPHICS_TEXTURE_FILTER_LINEAR,
				.mag_filter = GS_GRAPHICS_TEXTURE_FILTER_LINEAR,
				.mip_filter = GS_GRAPHICS_TEXTURE_FILTER_LINEAR,
				.num_mips   = 1,
				.data	    = map->lightmaps.data[i].map});
	}
}

void _bsp_load_lightvols(bsp_map_t *map)
{
}

void _bsp_create_patch(bsp_map_t *map, bsp_face_lump_t face)
{
	bsp_patch_t patch = {
		.width	      = face.size[0],
		.height	      = face.size[1],
		.lightmap_idx = face.lm_index,
		.texture_idx  = face.texture,
	};

	uint32_t num_patches_x = (patch.width - 1) >> 1;
	uint32_t num_patches_y = (patch.height - 1) >> 1;

	gs_dyn_array_reserve(patch.quadratic_patches, num_patches_x * num_patches_y);
	// not using push, increment size ourselves
	gs_dyn_array_head(patch.quadratic_patches)->size = num_patches_x * num_patches_y;

	for (size_t x = 0; x < num_patches_x; x++)
	{
		for (size_t y = 0; y < num_patches_y; y++)
		{
			uint32_t patch_idx = y * num_patches_x + x;

			bsp_quadratic_patch_t quadratic = {
				.tesselation = 8,
			};

			// Get the 9 vertices used as control points for this quadratic patch.
			for (size_t row = 0; row < 3; row++)
			{
				for (size_t col = 0; col < 3; col++)
				{
					uint32_t control_point_idx = row * 3 + col;
					// I understood this index when I wrote it but didn't add comments...
					// Let's just hope I never have to debug this.
					//                    offset               ???                             ???
					uint32_t vertex_idx			    = face.first_vertex + (2 * y * patch.width + 2 * x) + (row * patch.width + col);
					quadratic.control_points[control_point_idx] = map->vertices.data[vertex_idx];
				}
			}

			gs_dyn_array_set_data_i(&patch.quadratic_patches, &quadratic, sizeof(bsp_quadratic_patch_t), patch_idx);
			bsp_quadratic_patch_tesselate(&patch.quadratic_patches[patch_idx]);
		}
	}

	uint32_t temp = gs_dyn_array_size(patch.quadratic_patches);
	gs_dyn_array_push(map->patches, patch);
}

void _bsp_map_create_buffers(bsp_map_t *map)
{
	map->bsp_graphics_index_arr = gs_dyn_array_new(uint32_t);
	map->bsp_graphics_vert_arr  = gs_dyn_array_new(bsp_vert_lump_t);

	// Add regular faces
	gs_dyn_array_reserve(map->bsp_graphics_vert_arr, map->vertices.count);
	gs_dyn_array_push_data(&map->bsp_graphics_vert_arr, map->vertices.data, map->vertices.count * sizeof(bsp_vert_lump_t));
	gs_dyn_array_head(map->bsp_graphics_vert_arr)->size = map->vertices.count;
	for (size_t i = 0; i < gs_dyn_array_size(map->render_faces); i++)
	{
		if (map->render_faces[i].type != BSP_FACE_TYPE_PATCH)
		{
			bsp_face_lump_t face = map->faces.data[map->render_faces[i].index];
			int32_t first_index  = face.first_index;
			int32_t first_vertex = face.first_vertex;

			map->render_faces[i].first_ibo_index = gs_dyn_array_size(map->bsp_graphics_index_arr);
			map->render_faces[i].num_ibo_indices = face.num_indices;

			for (size_t j = 0; j < face.num_indices; j++)
			{
				gs_dyn_array_push(map->bsp_graphics_index_arr, first_vertex + map->indices.data[first_index + j].offset);
			}
		}
	}

	// Add patches
	uint32_t index_offset;
	for (size_t i = 0; i < gs_dyn_array_size(map->render_faces); i++)
	{
		if (map->render_faces[i].type == BSP_FACE_TYPE_PATCH)
		{
			bsp_patch_t patch		     = map->patches[map->render_faces[i].index];
			map->render_faces[i].first_ibo_index = gs_dyn_array_size(map->bsp_graphics_index_arr);

			for (size_t j = 0; j < gs_dyn_array_size(patch.quadratic_patches); j++)
			{
				bsp_quadratic_patch_t quadratic = patch.quadratic_patches[j];
				index_offset			= gs_dyn_array_size(map->bsp_graphics_vert_arr);

				for (size_t k = 0; k < gs_dyn_array_size(quadratic.vertices); k++)
				{
					gs_dyn_array_push(map->bsp_graphics_vert_arr, quadratic.vertices[k]);
				}

				for (size_t k = 0; k < gs_dyn_array_size(quadratic.indices); k++)
				{
					gs_dyn_array_push(map->bsp_graphics_index_arr, quadratic.indices[k] + index_offset);
				}
			}

			map->render_faces[i].num_ibo_indices = gs_dyn_array_size(map->bsp_graphics_index_arr) - map->render_faces[i].first_ibo_index;
		}
	}

	// Index buffer
	map->bsp_graphics_ibo = gs_graphics_index_buffer_create(
		&(gs_graphics_index_buffer_desc_t){
			.data  = map->bsp_graphics_index_arr,
			.size  = sizeof(uint32_t) * gs_dyn_array_size(map->bsp_graphics_index_arr),
			.usage = GS_GRAPHICS_BUFFER_USAGE_STATIC,
		});

	// Vertex buffer
	map->bsp_graphics_vbo = gs_graphics_vertex_buffer_create(
		&(gs_graphics_vertex_buffer_desc_t){
			.data  = map->bsp_graphics_vert_arr,
			.size  = sizeof(bsp_vert_lump_t) * gs_dyn_array_size(map->bsp_graphics_vert_arr),
			.usage = GS_GRAPHICS_BUFFER_USAGE_STATIC,
		});
}

void bsp_map_update(bsp_map_t *map, gs_camera_t *cam, const gs_vec2 fb)
{
	mg_time_manager_vis_start();

	int32_t leaf = _bsp_find_camera_leaf(map, cam->transform.position);
	if (leaf != map->previous_leaf)
	{
		// TODO: recalc PVS, move from _bsp_calculate_visible_faces
	}

	_bsp_calculate_visible_faces(map, leaf, cam, fb);
	map->previous_leaf = leaf;

	mg_time_manager_vis_end();
}

void bsp_map_render_immediate(bsp_map_t *map, gs_immediate_draw_t *gsi, gs_camera_t *cam)
{
	gsi_camera(gsi, cam, g_renderer->fb_size.x, g_renderer->fb_size.y);
	gsi_depth_enabled(gsi, true);
	// gsi_face_cull_enabled(gsi, true);

	for (size_t i = 0; i < gs_dyn_array_size(map->render_faces); i++)
	{
		if (!map->render_faces[i].visible) continue;

		bsp_face_renderable_t bsp_face = map->render_faces[i];
		int32_t index		       = bsp_face.index;

		if (bsp_face.type == BSP_FACE_TYPE_PATCH)
		{
			bsp_patch_t patch = map->patches[index];

			if (patch.texture_idx >= 0 && map->texture_assets.data[patch.texture_idx] != NULL && gs_handle_is_valid(map->texture_assets.data[patch.texture_idx]->hndl))
			{
				gsi_texture(gsi, map->texture_assets.data[patch.texture_idx]->hndl);
			}
			else
			{
				gsi_texture(gsi, map->missing_texture);
			}

			for (size_t j = 0; j < gs_dyn_array_size(patch.quadratic_patches); j++)
			{
				bsp_quadratic_patch_t quadratic = patch.quadratic_patches[j];

				for (size_t k = 0; k < gs_dyn_array_size(quadratic.indices) - 2; k += 3)
				{
					uint32_t index1 = quadratic.indices[k + 0];
					uint32_t index2 = quadratic.indices[k + 1];
					uint32_t index3 = quadratic.indices[k + 2];

					gsi_trianglevxmc(
						gsi,
						quadratic.vertices[index1].position,
						quadratic.vertices[index2].position,
						quadratic.vertices[index3].position,
						quadratic.vertices[index1].tex_coord,
						quadratic.vertices[index2].tex_coord,
						quadratic.vertices[index3].tex_coord,
						GS_COLOR_WHITE,
						GS_COLOR_WHITE,
						GS_COLOR_WHITE,
						GS_GRAPHICS_PRIMITIVE_TRIANGLES);
				}
			}
		}
		else
		{
			bsp_face_lump_t face = map->faces.data[index];
			int32_t first_index  = face.first_index;
			int32_t first_vertex = face.first_vertex;

			if (face.texture >= 0 && map->texture_assets.data[face.texture] != NULL && gs_handle_is_valid(map->texture_assets.data[face.texture]->hndl))
			{
				gsi_texture(gsi, map->texture_assets.data[face.texture]->hndl);
			}
			else
			{
				gsi_texture(gsi, map->missing_texture);
			}

			for (size_t j = 0; j < face.num_indices - 2; j += 3)
			{
				int32_t offset1 = map->indices.data[first_index + j + 0].offset;
				int32_t offset2 = map->indices.data[first_index + j + 1].offset;
				int32_t offset3 = map->indices.data[first_index + j + 2].offset;

				bsp_vert_lump_t vert1 = map->vertices.data[first_vertex + offset1];
				bsp_vert_lump_t vert2 = map->vertices.data[first_vertex + offset2];
				bsp_vert_lump_t vert3 = map->vertices.data[first_vertex + offset3];

				gsi_trianglevxmc(
					gsi,
					vert1.position,
					vert2.position,
					vert3.position,
					vert1.tex_coord,
					vert2.tex_coord,
					vert3.tex_coord,
					GS_COLOR_WHITE,
					GS_COLOR_WHITE,
					GS_COLOR_WHITE,
					GS_GRAPHICS_PRIMITIVE_TRIANGLES);
			}
		}
	}
}

void bsp_map_render(bsp_map_t *map, gs_camera_t *cam, gs_handle(gs_graphics_renderpass_t) rp, gs_command_buffer_t *cb, const gs_vec2 fb)
{
	mg_time_manager_bsp_start();

	bool wireframe = mg_cvar("r_wireframe")->value.i;

	// Clear desc
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

	// Uniforms that don't chang per face
	gs_mat4 u_proj = mg_camera_get_view_projection(cam, (s32)fb.x, (s32)fb.y);

	// Uniform binds
	gs_graphics_bind_uniform_desc_t uniforms[] = {
		{
			.uniform = map->bsp_graphics_u_proj,
			.data	 = &u_proj,
			.binding = 0, // VERTEX
		},
	};

	// Vertex buffer binds
	gs_graphics_bind_vertex_buffer_desc_t vbos[] = {
		{.buffer = map->bsp_graphics_vbo},
	};

	// Index buffer binds
	gs_graphics_bind_index_buffer_desc_t ibos[] = {
		{.buffer = map->bsp_graphics_ibo},
	};

	// Construct binds
	gs_graphics_bind_desc_t binds = {
		.vertex_buffers = {
			.desc = vbos,
			.size = sizeof(vbos),
		},
		.index_buffers = {
			.desc = ibos,
			.size = sizeof(ibos),
		},
		.uniforms = {
			.desc = uniforms,
			.size = sizeof(uniforms),
		},
	};

	// Render
	gs_graphics_renderpass_begin(cb, rp);
	gs_graphics_set_viewport(cb, 0, 0, (int32_t)fb.x, (int32_t)fb.y);
	gs_graphics_clear(cb, &clear);
	gs_graphics_pipeline_bind(cb, wireframe ? map->bsp_graphics_wire_pipe : map->bsp_graphics_pipe);
	gs_graphics_apply_bindings(cb, &binds);

	// Draw faces
	int32_t texture_index;
	int32_t lm_index;
	uint32_t vis_index;
	bsp_face_renderable_t bsp_face;
	for (size_t i = 0; i < gs_dyn_array_size(map->render_faces); i++)
	{
		if (!map->render_faces[i].visible) continue;

		bsp_face      = map->render_faces[i];
		texture_index = map->faces.data[bsp_face.index].texture;
		lm_index      = map->faces.data[bsp_face.index].lm_index;

		if (texture_index >= 0 && map->texture_assets.data[texture_index] == NULL || !gs_handle_is_valid(map->texture_assets.data[texture_index]->hndl))
		{
			texture_index = -1;
		}
		if (lm_index >= 0 && !gs_handle_is_valid(map->lightmap_textures.data[lm_index]))
		{
			lm_index = -1;
		}

		// Face specific uniforms
		gs_graphics_bind_uniform_desc_t face_uniforms[] = {
			// TEXTURE OR COLOR
			{0},
			// LIGHTMAP
			{0},
		};

		uint8_t uniform_count = wireframe ? 1 : 2;

		gs_vec4_t color = gs_v4(0, 0, 0, 1.0);

		if (wireframe)
		{
			switch (map->faces.data[bsp_face.index].type)
			{
			case BSP_FACE_TYPE_POLYGON:
				color.y = 1.0;
				break;

			case BSP_FACE_TYPE_MESH:
				color.z = 1.0;
				break;

			case BSP_FACE_TYPE_PATCH:
				color.x = 1.0;
				break;

			default:
				color.x = 0.5;
				color.y = 0.5;
				color.z = 0.5;
			}

			face_uniforms[0] = (gs_graphics_bind_uniform_desc_t){
				.uniform = map->bsp_graphics_u_color,
				.data	 = &color,
				.binding = 0, // FRAGMENT
			};
		}
		else
		{
			face_uniforms[0] = (gs_graphics_bind_uniform_desc_t){
				.uniform = map->bsp_graphics_u_tex,
				.data	 = texture_index >= 0 ? &map->texture_assets.data[texture_index]->hndl : &map->missing_texture,
				.binding = 0, // FRAGMENT
			};
			face_uniforms[1] = (gs_graphics_bind_uniform_desc_t){
				.uniform = map->bsp_graphics_u_lm,
				.data	 = lm_index >= 0 ? &map->lightmap_textures.data[lm_index] : &map->missing_lm_texture,
				.binding = 1, // FRAGMENT
			};
		}

		// Bind uniforms
		gs_graphics_bind_desc_t face_binds = {
			.uniforms = {
				.desc = face_uniforms,
				.size = sizeof(gs_graphics_bind_uniform_desc_t) * uniform_count,
			},
		};
		gs_graphics_apply_bindings(cb, &face_binds);

		// Draw face
		gs_graphics_draw(
			cb,
			&(gs_graphics_draw_desc_t){
				.start = (size_t)(intptr_t)(bsp_face.first_ibo_index * sizeof(uint32_t)),
				.count = (size_t)bsp_face.num_ibo_indices,
			});
	}

	gs_graphics_renderpass_end(cb);

	mg_time_manager_bsp_end();
}

void bsp_map_find_spawn_point(bsp_map_t *map, gs_vec3 *position, float32_t *yaw)
{
	gs_dyn_array(bsp_entity_t) spawns = gs_dyn_array_new(bsp_entity_t);

	for (size_t i = 0; i < gs_dyn_array_size(map->entities); i++)
	{
		bsp_entity_t ent = map->entities[i];
		char *classname	 = bsp_entity_get_value(&ent, "classname");
		if (strcmp(classname, "info_player_deathmatch") == 0 || strcmp(classname, "info_player_start") == 0)
		{
			gs_dyn_array_push(spawns, ent);
		}
	}

	if (gs_dyn_array_size(spawns) == 0)
	{
		gs_dyn_array_free(spawns);
		return;
	}

	uint32_t spawn_index = rand_range(0, gs_dyn_array_size(spawns) - 1);

	// Get position
	char *temp = bsp_entity_get_value(&spawns[spawn_index], "origin");
	gs_assert(temp != NULL);

	char *temp2	   = mg_duplicate_string(temp);
	char *num_str	   = strtok(temp2, " ");
	uint32_t vec_index = 0;

	while (num_str != NULL)
	{
		gs_assert(vec_index < 3);
		position->xyz[vec_index] = strtol(num_str, (char **)NULL, 10);
		vec_index++;
		num_str = strtok(0, " ");
	}
	gs_free(temp2);
	gs_assert(vec_index == 3);

	// Get yaw
	temp = bsp_entity_get_value(&spawns[spawn_index], "angle");
	if (temp == NULL)
		*yaw = 0;
	else
		*yaw = strtof(temp, NULL);

	gs_dyn_array_free(spawns);
}

void bsp_map_free(bsp_map_t *map)
{
	if (map == NULL)
	{
		return;
	}

	/*==== Runtime data ====*/

	if (map->valid)
	{
		gs_graphics_vertex_buffer_destroy(map->bsp_graphics_vbo);
		gs_graphics_index_buffer_destroy(map->bsp_graphics_ibo);
		gs_graphics_pipeline_destroy(map->bsp_graphics_pipe);
		gs_graphics_pipeline_destroy(map->bsp_graphics_wire_pipe);
		gs_graphics_uniform_destroy(map->bsp_graphics_u_proj);
		gs_graphics_uniform_destroy(map->bsp_graphics_u_tex);
		gs_graphics_uniform_destroy(map->bsp_graphics_u_lm);
		gs_graphics_uniform_destroy(map->bsp_graphics_u_color);
		gs_dyn_array_free(map->bsp_graphics_index_arr);
		gs_dyn_array_free(map->bsp_graphics_vert_arr);

		for (size_t i = 0; i < gs_dyn_array_size(map->entities); i++)
		{
			bsp_entity_free(&map->entities[i]);
		}
		gs_dyn_array_free(map->entities);

		for (size_t i = 0; i < gs_dyn_array_size(map->patches); i++)
		{
			bsp_patch_free(&map->patches[i]);
		}
		gs_dyn_array_free(map->patches);
		gs_dyn_array_free(map->render_faces);

		map->patches	  = NULL;
		map->render_faces = NULL;

		// data contents will be freed by texture manager
		gs_free(map->texture_assets.data);
		map->texture_assets.data = NULL;

		for (size_t i = 0; i < map->lightmap_textures.count; i++)
		{
			gs_graphics_texture_destroy(map->lightmap_textures.data[i]);
		}
		gs_free(map->lightmap_textures.data);
		map->lightmap_textures.data = NULL;

		gs_graphics_texture_destroy(map->missing_texture);
		gs_graphics_texture_destroy(map->missing_lm_texture);
	}

	/*==== File data ====*/

	gs_free(map->entity_lump.ents);
	gs_free(map->textures.data);
	gs_free(map->planes.data);
	gs_free(map->nodes.data);
	gs_free(map->leaves.data);
	gs_free(map->leaf_faces.data);
	gs_free(map->leaf_brushes.data);
	gs_free(map->models.data);
	gs_free(map->brushes.data);
	gs_free(map->brush_sides.data);
	gs_free(map->vertices.data);
	gs_free(map->indices.data);
	gs_free(map->effects.data);
	gs_free(map->faces.data);
	gs_free(map->lightmaps.data);
	gs_free(map->lightvols.data);
	gs_free(map->visdata.vecs);
	gs_free(map->name);

	map->entity_lump.ents  = NULL;
	map->textures.data     = NULL;
	map->planes.data       = NULL;
	map->nodes.data	       = NULL;
	map->leaves.data       = NULL;
	map->leaf_faces.data   = NULL;
	map->leaf_brushes.data = NULL;
	map->models.data       = NULL;
	map->brushes.data      = NULL;
	map->brush_sides.data  = NULL;
	map->vertices.data     = NULL;
	map->indices.data      = NULL;
	map->effects.data      = NULL;
	map->faces.data	       = NULL;
	map->lightmaps.data    = NULL;
	map->lightvols.data    = NULL;
	map->visdata.vecs      = NULL;
	map->name	       = NULL;

	gs_free(map);
}

int32_t _bsp_find_camera_leaf(bsp_map_t *map, gs_vec3 view_position)
{
	int32_t leaf_index = 0;

	while (leaf_index >= 0)
	{
		bsp_plane_lump_t plane = map->planes.data[map->nodes.data[leaf_index].plane];

		// children[0] - front node; children[1] - back node
		if (point_in_front_of_plane(plane.normal, plane.dist, view_position))
		{
			leaf_index = map->nodes.data[leaf_index].children[0];
		}
		else
		{
			leaf_index = map->nodes.data[leaf_index].children[1];
		}
	}

	return ~leaf_index;
}

void _bsp_calculate_visible_faces(bsp_map_t *map, int32_t leaf, gs_camera_t *cam, const gs_vec2 fb)
{
	uint32_t culled_leaves_pvs     = 0;
	uint32_t culled_leaves_frustum = 0;
	uint32_t visible_leaves	       = 0;
	uint32_t visible_patches       = 0;
	uint32_t visible_faces	       = 0;
	uint32_t visible_vertices      = 0;
	uint32_t visible_indices       = 0;
	int32_t view_cluster	       = map->leaves.data[leaf].cluster;

	for (size_t i = 0; i < gs_dyn_array_size(map->render_faces); i++)
	{
		map->render_faces[i].visible = false;
	}

	for (size_t i = 0; i < map->leaves.count; i++)
	{
		bsp_leaf_lump_t lump = map->leaves.data[i];

		// TODO:
		// Store PVS, don't run every frame if leaf doesn't change
		if (!_bsp_cluster_visible(map, view_cluster, lump.cluster))
		{
			culled_leaves_pvs++;
			continue;
		}

		// Frustum culling using lump.mins and lump.maxs
		gs_mat4 proj	       = mg_camera_get_view_projection(cam, (s32)fb.x, (s32)fb.y);
		mg_camera_frustum_t fr = mg_camera_get_frustum_planes(proj, false);

		if (!mg_camera_aabb_in_frustum(
			    fr,
			    gs_v3(lump.mins[0], lump.mins[1], lump.mins[2]),
			    gs_v3(lump.maxs[0], lump.maxs[1], lump.maxs[2])))
		{
			culled_leaves_frustum++;
			continue;
		}

		visible_leaves++;

		// Add faces in this leaf to visible set
		for (size_t j = 0; j < lump.num_leaf_faces; j++)
		{
			int32_t idx		   = map->leaf_faces.data[lump.first_leaf_face + j].face;
			bsp_face_renderable_t face = map->render_faces[idx];

			// Same face can be in multiple leaves
			if (face.visible)
			{
				continue;
			}

			map->render_faces[idx].visible = true;

			// TODO billboards
			if (face.type == BSP_FACE_TYPE_BILLBOARD)
			{
				continue;
			}

			if (face.type == BSP_FACE_TYPE_PATCH)
			{
				visible_patches++;
				bsp_patch_t patch = map->patches[face.index];
				for (size_t k = 0; k < gs_dyn_array_size(patch.quadratic_patches); k++)
				{
					visible_vertices += gs_dyn_array_size(patch.quadratic_patches[k].vertices);
					visible_indices += gs_dyn_array_size(patch.quadratic_patches[k].indices);
				}
			}
			else
			{
				visible_faces++;
				visible_vertices += map->faces.data[face.index].num_vertices;
				visible_vertices += map->faces.data[face.index].num_indices;
			}
		}
	}

	map->stats.culled_leaves_pvs	 = culled_leaves_pvs;
	map->stats.culled_leaves_frustum = culled_leaves_frustum;
	map->stats.visible_leaves	 = visible_leaves;
	map->stats.visible_vertices	 = visible_vertices;
	map->stats.visible_indices	 = visible_indices;
	map->stats.visible_faces	 = visible_faces;
	map->stats.visible_patches	 = visible_patches;
	map->stats.current_leaf		 = leaf;
}

bool32_t _bsp_cluster_visible(bsp_map_t *map, int32_t view_cluster, int32_t test_cluster)
{
	if (test_cluster < 0)
	{
		// Outside the map or invalid
		return false;
	}

	if (map->visdata.num_vecs == 0 || view_cluster < 0)
	{
		return true;
	}

	// int32_t idx = test_cluster * map->visdata.size_vecs + view_cluster / 8;
	// return (map->visdata.vecs[idx] & (1 << (view_cluster % 8))) != 0;

	int32_t idx = test_cluster * map->visdata.size_vecs + (view_cluster >> 3);
	return (map->visdata.vecs[idx] & (1 << (view_cluster & 7))) != 0;
}

bsp_lightvol_lump_t bsp_get_lightvol(bsp_map_t *map, gs_vec3 position, gs_vec3 *center)
{
	// Light volumes are 64x64x128 units in size.
	// This is how many volumes there are per axis.
	uint32_t num_x = floor(map->models.data[0].maxs.x / 64) - ceil(map->models.data[0].mins.x / 64) + 1;
	uint32_t num_y = floor(map->models.data[0].maxs.y / 64) - ceil(map->models.data[0].mins.y / 64) + 1;
	uint32_t num_z = floor(map->models.data[0].maxs.z / 128) - ceil(map->models.data[0].mins.z / 128) + 1;

	// Get our position in the map
	gs_vec3 map_size	  = gs_vec3_sub(map->models.data[0].maxs, map->models.data[0].mins);
	gs_vec3 position_fraction = gs_vec3_div(gs_vec3_sub(position, map->models.data[0].mins), map_size);

	if (
		position_fraction.x < 0 ||
		position_fraction.y < 0 ||
		position_fraction.z < 0 ||
		position_fraction.x > 1.0f ||
		position_fraction.y > 1.0f ||
		position_fraction.z > 1.0f)
	{
		return (bsp_lightvol_lump_t){0};
	}

	uint32_t pos_x = (uint32_t)(num_x * position_fraction.x);
	uint32_t pos_y = (uint32_t)(num_y * position_fraction.y);
	uint32_t pos_z = (uint32_t)(num_z * position_fraction.z);
	uint32_t position_index =
		pos_x +
		pos_y * num_x +
		pos_z * num_x * num_y;

	if (center != NULL)
	{
		*center = gs_v3(pos_x * 64.0f, pos_y * 64.0f, pos_z * 128.0f);
	}

	return map->lightvols.data[position_index];
}

// TODO: still some funkiness when moving between lightvols,
// ideally make interp good enough to not have to support
// multiple directional lights.
// This also takes way too much time each frame!
mg_renderer_light_t bsp_sample_lightvol(bsp_map_t *map, gs_vec3 position)
{
	// Get our position in the map
	gs_vec3 position_relative = gs_vec3_sub(position, map->models.data[0].mins);

	mg_renderer_light_t light = {0};
	bsp_lightvol_lump_t lump;
	gs_vec3 lump_center = gs_default_val();
	float frac;
	float distance;
	float total = 0;
	float phi   = 0;
	float theta = 0;

	// How many volumes to sample in each direction on all axis.
	// Total volumes = (2 * samples_xy + 1)^2 * (2 * samples_z + 1)
	int32_t samples_xy = 1;
	int32_t samples_z  = 1;

	for (int32_t x = -64 * samples_xy; x < 64 * samples_xy + 1; x += 64)
	{
		for (int32_t y = -64 * samples_xy; y < 64 * samples_xy + 1; y += 64)
		{
			for (int32_t z = -128 * samples_z; z < 128 * samples_z + 1; z += 128)
			{
				lump = bsp_get_lightvol(map, gs_vec3_add(position, gs_v3(x, y, z)), &lump_center);

				if (lump.ambient[0] == 0 && lump.ambient[1] == 0 && lump.ambient[2] == 0)
				{
					// Inside a wall or outside the map
					continue;
				}

				distance = gs_vec3_dist(position_relative, lump_center);
				frac	 = 1.0f - distance / sqrtf(powf(samples_z * 128, 2) + 2 * powf(samples_xy * 64, 2));
				if (frac < 0)
				{
					continue;
				}
				total += frac;

				light.ambient.x += frac * lump.ambient[0];
				light.ambient.y += frac * lump.ambient[1];
				light.ambient.z += frac * lump.ambient[2];

				light.directional.x += frac * lump.directional[0];
				light.directional.y += frac * lump.directional[1];
				light.directional.z += frac * lump.directional[2];

				// The higher the intensity of the light at this point in space,
				// the more it affects the direction vector.
				float intensity = gs_vec3_len(light.directional) / 1325.0f; // sqrt(3 * 255^2)

				// clang-format off
                light.direction = gs_vec3_add(
                    light.direction,
                    gs_vec3_scale(
                        mg_sphere_to_normal(lump.dir), 
                        frac * intensity
                    )
                );
				// clang-format on
			}
		}
	}

	if (total == 0)
	{
		return light;
	}

	// Normalize
	total		  = 1.0f / total;
	light.ambient	  = gs_vec3_scale(light.ambient, total / 255.0f);
	light.directional = gs_vec3_scale(light.directional, total / 255.0f);
	light.direction	  = gs_vec3_norm(light.direction);

	return light;
}