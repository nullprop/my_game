/*================================================================
	* graphics/model.c
	*
	* Copyright (c) 2021 Lauri Räsänen
	* ================================

	Q3 MD3 version 15 loading + modified animation.cfg.
=================================================================*/

#include "model.h"
#include "../game/console.h"
#include "../util/render.h"
#include "../util/string.h"
#include "../util/transform.h"
#include "texture_manager.h"

// shorthand util for failing during MD3 load
b32 _mg_load_md3_fail(gs_byte_buffer_t *buffer, char *msg)
{
	mg_println("mg_load_md3() failed: %s", msg);
	gs_byte_buffer_free(buffer);
	return false;
}

bool mg_load_md3(char *filename, md3_t *model)
{
	mg_println("mg_load_md3() loading: '%s'", filename);

	if (!gs_util_file_exists(filename))
	{
		mg_println("mg_load_md3() failed: file not found '%s'", filename);
		return false;
	}

	gs_byte_buffer_t buffer = gs_byte_buffer_new();
	gs_byte_buffer_read_from_file(&buffer, filename);

	// read header
	gs_byte_buffer_read(&buffer, md3_header_t, &model->header);

	// validate header
	if (memcmp(model->header.magic, MD3_MAGIC, 4) != 0 || model->header.version != MD3_VERSION)
		return _mg_load_md3_fail(&buffer, "invalid header");

	// read frames
	size_t sz	= sizeof(md3_frame_t) * model->header.num_frames;
	model->frames	= gs_malloc(sz);
	buffer.position = model->header.off_frames;
	gs_byte_buffer_read_bulk(&buffer, &model->frames, sz);

	// read tags
	sz		= sizeof(md3_tag_t) * model->header.num_tags;
	model->tags	= gs_malloc(sz);
	buffer.position = model->header.off_tags;
	gs_byte_buffer_read_bulk(&buffer, &model->tags, sz);

	// read surfaces
	sz		= sizeof(md3_surface_t) * model->header.num_surfaces;
	model->surfaces = gs_malloc(sz);
	memset(model->surfaces, 0, sz);
	buffer.position = model->header.off_surfaces;
	for (size_t i = 0; i < model->header.num_surfaces; i++)
	{
		// Offset to the start of this surface
		uint32_t off_start = buffer.position;

		md3_surface_t *surf = &model->surfaces[i];

		surf->magic = gs_malloc(4);
		gs_byte_buffer_read_bulk(&buffer, &surf->magic, 4);

		// validate magic
		if (memcmp(surf->magic, MD3_MAGIC, 4) != 0)
			return _mg_load_md3_fail(&buffer, "invalid magic in surface");

		surf->name = gs_malloc(64);
		gs_byte_buffer_read_bulk(&buffer, &surf->name, 64);
		gs_byte_buffer_read(&buffer, int32_t, &surf->flags);
		gs_byte_buffer_read(&buffer, int32_t, &surf->num_frames);

		// validate frames
		if (surf->num_frames != model->header.num_frames)
			return _mg_load_md3_fail(&buffer, "invalid number of frames in surface");

		gs_byte_buffer_read(&buffer, int32_t, &surf->num_shaders);
		gs_byte_buffer_read(&buffer, int32_t, &surf->num_verts);
		gs_byte_buffer_read(&buffer, int32_t, &surf->num_tris);
		gs_byte_buffer_read(&buffer, int32_t, &surf->off_tris);
		gs_byte_buffer_read(&buffer, int32_t, &surf->off_shaders);
		gs_byte_buffer_read(&buffer, int32_t, &surf->off_texcoords);
		gs_byte_buffer_read(&buffer, int32_t, &surf->off_verts);
		gs_byte_buffer_read(&buffer, int32_t, &surf->off_end);

		// shaders
		sz		= sizeof(md3_shader_t) * surf->num_shaders;
		surf->shaders	= gs_malloc(sz);
		buffer.position = off_start + surf->off_shaders;
		gs_byte_buffer_read_bulk(&buffer, &surf->shaders, sz);

		// triangles
		sz		= sizeof(md3_triangle_t) * surf->num_tris;
		surf->triangles = gs_malloc(sz);
		buffer.position = off_start + surf->off_tris;
		gs_byte_buffer_read_bulk(&buffer, &surf->triangles, sz);

		// texcoords
		sz		= sizeof(md3_texcoord_t) * surf->num_shaders * surf->num_verts;
		surf->texcoords = gs_malloc(sz);
		buffer.position = off_start + surf->off_texcoords;
		gs_byte_buffer_read_bulk(&buffer, &surf->texcoords, sz);

		// vertices
		sz		= sizeof(md3_texcoord_t) * surf->num_verts * surf->num_frames;
		surf->vertices	= gs_malloc(sz);
		buffer.position = off_start + surf->off_verts;
		gs_byte_buffer_read_bulk(&buffer, &surf->vertices, sz);

		// Renderable vertices
		surf->render_vertices = gs_malloc(sizeof(mg_md3_render_vertex_t) * surf->num_verts * surf->num_frames);
		for (size_t j = 0; j < surf->num_verts * surf->num_frames; j++)
		{
			// TODO: multiple shaders?
			int16_t shader_index		    = 0;
			surf->render_vertices[j].position.x = surf->vertices[j].x / MD3_SCALE;
			surf->render_vertices[j].position.y = surf->vertices[j].y / MD3_SCALE;
			surf->render_vertices[j].position.z = surf->vertices[j].z / MD3_SCALE;

			surf->render_vertices[j].normal = mg_int16_to_vec3(surf->vertices[j].normal);

			// Loop texcoords for each frame
			size_t texcoord_index		    = ((shader_index + 1) * j) % (surf->num_shaders * surf->num_verts);
			surf->render_vertices[j].texcoord.x = surf->texcoords[texcoord_index].u;
			surf->render_vertices[j].texcoord.y = surf->texcoords[texcoord_index].v;
		}

		// Vertex buffers
		surf->vbos = gs_malloc(sizeof(gs_handle_gs_graphics_vertex_buffer_t) * surf->num_frames);
		for (size_t j = 0; j < surf->num_frames; j++)
		{
			gs_graphics_vertex_buffer_desc_t vdesc = gs_default_val();
			vdesc.data			       = surf->render_vertices + j * surf->num_verts;
			vdesc.size			       = sizeof(mg_md3_render_vertex_t) * surf->num_verts;
			surf->vbos[j]			       = gs_graphics_vertex_buffer_create(&vdesc);
		}

		// Index buffer
		gs_graphics_index_buffer_desc_t idesc = gs_default_val();
		idesc.data			      = surf->triangles;
		idesc.size			      = sizeof(md3_triangle_t) * surf->num_tris;
		surf->ibo			      = gs_graphics_index_buffer_create(&idesc);

		// Textures
		surf->textures = gs_malloc(sizeof(gs_asset_texture_t *) * surf->num_shaders);
		for (size_t j = 0; j < surf->num_shaders; j++)
		{
			// FIXME: why do shader names start with null?
			// \0odels/players/...
			if (surf->shaders[j].name[0] == '\0') surf->shaders[j].name[0] = 'm';

			surf->textures[j] = mg_texture_manager_get(surf->shaders[j].name);
		}

		// Seek to next surface
		buffer.position = off_start + surf->off_end;
	}

	// Animations from <model>_animation.cfg
	model->animations = gs_dyn_array_new(mg_md3_animation_t);
	char *model_path  = mg_path_remove_ext(filename);
	char *cfg_path	  = gs_malloc(gs_string_length(model_path) + 15);
	memset(cfg_path, '\0', sizeof(cfg_path));
	strcat(cfg_path, model_path);
	strcat(cfg_path, "_animation.cfg");

	if (gs_util_file_exists(cfg_path))
	{
		mg_println("mg_load_md3(): loading animations from '%s'", cfg_path);

		FILE *file = fopen(cfg_path, "r");
		if (file == NULL)
		{
			mg_println("WARN: failed to read animation file %s", cfg_path);
			return false;
		}

		char line[128];
		char *token;
		u8 num_parts = 0;
		u8 num_line  = 0;
		while (fgets(line, sizeof(line), file))
		{
			num_line++;

			// Empty line
			if (line[0] == '\n')
			{
				continue;
			}

			// Comment
			if (line[0] == '/' && line[1] == '/')
			{
				continue;
			}

			mg_md3_animation_t anim = (mg_md3_animation_t){};

			// Parse values delimited by space:
			// first frame, num frames, loop, frames per second, name
			num_parts = 0;
			token	  = strtok(&line, " ");
			while (token)
			{
				switch (num_parts)
				{
				case 0:
					anim.first_frame = strtol(token, (char **)NULL, 10);
					break;

				case 1:
					anim.num_frames = strtol(token, (char **)NULL, 10);
					break;

				case 2:
					anim.loop = strtol(token, (char **)NULL, 10);
					break;

				case 3:
					anim.fps = strtol(token, (char **)NULL, 10);
					break;

				case 4:
					strcat(anim.name, token);
					// Remove new line at the end
					for (size_t i = 0; i < 16; i++)
					{
						if (anim.name[i] == '\n')
						{
							anim.name[i] = '\0';
							break;
						}
					}

					break;

				default:
					mg_println("WARN: animation config line %zu has too many arguments", num_line);
					break;
				}

				num_parts++;

				token = strtok(0, " ");
			}

			// Check we got all
			if (num_parts < 5)
			{
				mg_println("WARN: animation config line %zu has too few arguments", num_line);
				continue;
			}

			gs_dyn_array_push(model->animations, anim);
		}

		fclose(file);
		mg_println("Config loaded");
	}
	else
	{
		mg_println("mg_load_md3(): no animation.cfg for model '%s' (%s)", filename, cfg_path);
	}

	gs_free(model_path);
	gs_free(cfg_path);

	gs_byte_buffer_free(&buffer);

	return true;
}

void mg_free_md3(md3_t *model)
{
	for (size_t i = 0; i < model->header.num_surfaces; i++)
	{
		gs_graphics_index_buffer_destroy(model->surfaces[i].ibo);

		for (size_t j = 0; j < model->surfaces[i].num_frames; j++)
		{
			gs_graphics_vertex_buffer_destroy(model->surfaces[i].vbos[j]);
		}

		gs_free(model->surfaces[i].magic);
		gs_free(model->surfaces[i].name);
		gs_free(model->surfaces[i].shaders);
		gs_free(model->surfaces[i].triangles);
		gs_free(model->surfaces[i].texcoords);
		gs_free(model->surfaces[i].vertices);
		gs_free(model->surfaces[i].render_vertices);
		gs_free(model->surfaces[i].vbos);
		// contents will be freed by texture manager
		gs_free(model->surfaces[i].textures);

		model->surfaces[i].magic	   = NULL;
		model->surfaces[i].name		   = NULL;
		model->surfaces[i].shaders	   = NULL;
		model->surfaces[i].triangles	   = NULL;
		model->surfaces[i].texcoords	   = NULL;
		model->surfaces[i].vertices	   = NULL;
		model->surfaces[i].render_vertices = NULL;
		model->surfaces[i].vbos		   = NULL;
		model->surfaces[i].textures	   = NULL;
	}

	gs_free(model->frames);
	gs_free(model->tags);
	gs_free(model->surfaces);

	model->frames	= NULL;
	model->tags	= NULL;
	model->surfaces = NULL;

	gs_dyn_array_free(model->animations);

	gs_free(model);
	model = NULL;
}
