/*================================================================
    * graphics/model.c
    *
    * Copyright (c) 2021 Lauri Räsänen
    * ================================

    Q3 MD3 version 15 loading.
=================================================================*/

#include "model.h"
#include "../util/render.h"
#include "../util/transform.h"

// shorthand util for failing during MD3 load
b32 _mg_load_md3_fail(gs_byte_buffer_t *buffer, char *msg)
{
    gs_println("mg_load_md3() failed: %s", msg);
    gs_byte_buffer_free(buffer);
    return false;
}

bool mg_load_md3(char *filename, md3_t *model)
{
    gs_println("mg_load_md3() loading: '%s'", filename);

    if (!gs_util_file_exists(filename))
    {
        gs_println("mg_load_md3() failed: file not found '%s'", filename);
        return false;
    }

    gs_byte_buffer_t buffer = gs_byte_buffer_new();
    gs_byte_buffer_read_from_file(&buffer, filename);

    // read header
    gs_byte_buffer_read(&buffer, md3_header_t, &model->header);

    // validate header
    if (!gs_string_compare_equal_n(model->header.magic, MD3_MAGIC, 4) || model->header.version != MD3_VERSION)
        return _mg_load_md3_fail(&buffer, "invalid header");

    // read frames
    size_t sz = sizeof(md3_frame_t) * model->header.num_frames;
    model->frames = gs_malloc(sz);
    buffer.position = model->header.off_frames;
    gs_byte_buffer_read_bulk(&buffer, &model->frames, sz);

    // read tags
    sz = sizeof(md3_tag_t) * model->header.num_tags;
    model->tags = gs_malloc(sz);
    buffer.position = model->header.off_tags;
    gs_byte_buffer_read_bulk(&buffer, &model->tags, sz);

    // read surfaces
    sz = sizeof(md3_surface_t) * model->header.num_surfaces;
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
        if (!gs_string_compare_equal_n(surf->magic, MD3_MAGIC, 4))
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
        sz = sizeof(md3_shader_t) * surf->num_shaders;
        surf->shaders = gs_malloc(sz);
        buffer.position = off_start + surf->off_shaders;
        gs_byte_buffer_read_bulk(&buffer, &surf->shaders, sz);

        // triangles
        sz = sizeof(md3_triangle_t) * surf->num_tris;
        surf->triangles = gs_malloc(sz);
        buffer.position = off_start + surf->off_tris;
        gs_byte_buffer_read_bulk(&buffer, &surf->triangles, sz);

        // texcoords
        sz = sizeof(md3_texcoord_t) * surf->num_shaders * surf->num_verts;
        surf->texcoords = gs_malloc(sz);
        buffer.position = off_start + surf->off_texcoords;
        gs_byte_buffer_read_bulk(&buffer, &surf->texcoords, sz);

        // vertices
        sz = sizeof(md3_texcoord_t) * surf->num_verts * surf->num_frames;
        surf->vertices = gs_malloc(sz);
        buffer.position = off_start + surf->off_verts;
        gs_byte_buffer_read_bulk(&buffer, &surf->vertices, sz);

        // Renderable vertices
        surf->render_vertices = gs_malloc(sizeof(mg_md3_render_vertex_t) * surf->num_verts * surf->num_frames);
        for (size_t j = 0; j < surf->num_verts * surf->num_frames; j++)
        {
            // TODO: multiple shaders?
            int16_t shader_index = 0;
            surf->render_vertices[j].position.x = surf->vertices[j].x / MD3_SCALE;
            surf->render_vertices[j].position.y = surf->vertices[j].y / MD3_SCALE;
            surf->render_vertices[j].position.z = surf->vertices[j].z / MD3_SCALE;

            surf->render_vertices[j].normal = mg_int16_to_vec3(surf->vertices[j].normal);

            // Loop texcoords for each frame
            size_t texcoord_index = ((shader_index + 1) * j) % (surf->num_shaders * surf->num_verts);
            surf->render_vertices[j].texcoord.x = surf->texcoords[texcoord_index].u;
            surf->render_vertices[j].texcoord.y = surf->texcoords[texcoord_index].v;
        }

        // Vertex buffers
        surf->vbos = gs_malloc(sizeof(gs_handle_gs_graphics_vertex_buffer_t) * surf->num_frames);
        for (size_t j = 0; j < surf->num_frames; j++)
        {
            gs_graphics_vertex_buffer_desc_t vdesc = gs_default_val();
            vdesc.data = surf->render_vertices + j * surf->num_verts;
            vdesc.size = sizeof(mg_md3_render_vertex_t) * surf->num_verts;
            surf->vbos[j] = gs_graphics_vertex_buffer_create(&vdesc);
        }

        // Index buffer
        gs_graphics_index_buffer_desc_t idesc = gs_default_val();
        idesc.data = surf->triangles;
        idesc.size = sizeof(md3_triangle_t) * surf->num_tris;
        surf->ibo = gs_graphics_index_buffer_create(&idesc);

        // Textures
        // TODO: don't load same texture multiple times; create manager
        surf->textures = gs_malloc(sizeof(gs_asset_texture_t) * surf->num_shaders);
        for (size_t i = 0; i < surf->num_shaders; i++)
        {
            // FIXME: why do shader names start with null?
            // \0odels/players/...
            if (surf->shaders[i].name[0] == '\0') surf->shaders[i].name[0] = 'm';
            mg_load_texture_asset(surf->shaders[i].name, &surf->textures[i]);
        }

        // Seek to next surface
        buffer.position = off_start + surf->off_end;
    }

    return true;
}

void mg_free_md3(md3_t *model)
{
    for (size_t i = 0; i < model->header.num_surfaces; i++)
    {
        gs_free(model->surfaces[i].magic);
        gs_free(model->surfaces[i].name);
        gs_free(model->surfaces[i].shaders);
        gs_free(model->surfaces[i].triangles);
        gs_free(model->surfaces[i].texcoords);
        gs_free(model->surfaces[i].vertices);
        gs_free(model->surfaces[i].render_vertices);
        gs_free(model->surfaces[i].textures);

        model->surfaces[i].magic = NULL;
        model->surfaces[i].name = NULL;
        model->surfaces[i].shaders = NULL;
        model->surfaces[i].triangles = NULL;
        model->surfaces[i].texcoords = NULL;
        model->surfaces[i].vertices = NULL;
        model->surfaces[i].render_vertices = NULL;
        model->surfaces[i].textures = NULL;
    }

    gs_free(model->frames);
    gs_free(model->tags);
    gs_free(model->surfaces);

    model->frames = NULL;
    model->tags = NULL;
    model->surfaces = NULL;
}
