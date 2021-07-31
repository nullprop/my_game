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

#include "../util/math.c"
#include "bsp_patch.c"
#include "bsp_shaders.h"
#include "bsp_types.h"

static gs_command_buffer_t bsp_graphics_cb = {0};
static gs_handle(gs_graphics_vertex_buffer_t) bsp_graphics_vbo = {0};
static gs_handle(gs_graphics_index_buffer_t) bsp_graphics_ibo = {0};
static gs_handle(gs_graphics_pipeline_t) bsp_graphics_pipe = {0};
static gs_handle(gs_graphics_shader_t) bsp_graphics_shader = {0};
static gs_handle(gs_graphics_uniform_t) bsp_graphics_u_proj = {0};
static gs_dyn_array(uint32_t) bsp_graphics_index_arr;

void _bsp_load_entities(bsp_map_t *map);
void _bsp_load_textures(bsp_map_t *map);
void _bsp_load_lightmaps(bsp_map_t *map);
void _bsp_load_lightvols(bsp_map_t *map);
void _bsp_create_patch(bsp_map_t *map, bsp_face_lump_t face);
void _bsp_create_index_buffer(bsp_map_t *map);
int32_t _bsp_find_camera_leaf(bsp_map_t *map, gs_vec3 view_position);
void _bsp_calculate_visible_faces(bsp_map_t *map, int32_t leaf);
bool32_t _bsp_cluster_visible(bsp_map_t *map, int32_t view_cluster, int32_t test_cluster);

void bsp_map_init(bsp_map_t *map)
{
    if (map->faces.count == 0)
    {
        return;
    }

    map->previous_leaf = uint32_max;

    // Init dynamic arrays
    gs_dyn_array_reserve(map->render_faces, map->faces.count);
    gs_dyn_array_reserve(map->visible_faces, map->faces.count);
    uint32_t patch_count;
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

    uint32_t face_array_idx = 0;
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

    // Static stats
    map->stats.total_vertices = map->vertices.count;
    map->stats.total_faces = face_array_idx;
    map->stats.total_patches = patch_array_idx;

    // Command buffer
    bsp_graphics_cb = gs_command_buffer_new();

    // Vertex buffer
    bsp_graphics_vbo = gs_graphics_vertex_buffer_create(
        &(gs_graphics_vertex_buffer_desc_t){
            .data = map->vertices.data,
            .size = sizeof(bsp_vert_lump_t) * map->vertices.count,
        });

    // Index buffer
    _bsp_map_create_index_buffer(map);
    /*
    bsp_graphics_ibo = gs_graphics_index_buffer_create(
        &(gs_graphics_index_buffer_desc_t){
            .data = map->indices.data,
            .size = sizeof(bsp_index_lump_t) * map->indices.count,
        });
    */

    // Shader source description
    gs_graphics_shader_source_desc_t sources[] = {
        (gs_graphics_shader_source_desc_t){.type = GS_GRAPHICS_SHADER_STAGE_VERTEX, .source = bsp_shader_vert_src},
        (gs_graphics_shader_source_desc_t){.type = GS_GRAPHICS_SHADER_STAGE_FRAGMENT, .source = bsp_shader_frag_src},
    };

    // Create shader
    bsp_graphics_shader = gs_graphics_shader_create(
        &(gs_graphics_shader_desc_t){
            .sources = sources,
            .size = sizeof(sources),
            .name = "bsp",
        });

    // Create uniforms
    bsp_graphics_u_proj = gs_graphics_uniform_create(
        &(gs_graphics_uniform_desc_t){
            .name = "u_proj",
            .layout = &(gs_graphics_uniform_layout_desc_t){
                .type = GS_GRAPHICS_UNIFORM_MAT4,
            },
        });

    // Pipeline vertex attributes
    gs_graphics_vertex_attribute_desc_t vattrs[] = {
        (gs_graphics_vertex_attribute_desc_t){.format = GS_GRAPHICS_VERTEX_ATTRIBUTE_FLOAT3, .name = "a_pos"},
        (gs_graphics_vertex_attribute_desc_t){.format = GS_GRAPHICS_VERTEX_ATTRIBUTE_FLOAT2, .name = "a_tex_coord"},
        (gs_graphics_vertex_attribute_desc_t){.format = GS_GRAPHICS_VERTEX_ATTRIBUTE_FLOAT2, .name = "a_lm_coord"},
        (gs_graphics_vertex_attribute_desc_t){.format = GS_GRAPHICS_VERTEX_ATTRIBUTE_FLOAT3, .name = "a_normal"},
        (gs_graphics_vertex_attribute_desc_t){.format = GS_GRAPHICS_VERTEX_ATTRIBUTE_UINT4, .name = "a_color"},
    };

    bsp_graphics_pipe = gs_graphics_pipeline_create(
        &(gs_graphics_pipeline_desc_t){
            .raster = {
                .shader = bsp_graphics_shader,
                .index_buffer_element_size = sizeof(uint32_t),
            },
            .layout = {
                .attrs = vattrs,
                .size = sizeof(vattrs),
            },
        });
}

void _bsp_load_entities(bsp_map_t *map)
{
}

void _bsp_load_textures(bsp_map_t *map)
{
    int32_t num_textures = map->header.dir_entries[BSP_LUMP_TYPE_TEXTURES].length / sizeof(bsp_texture_lump_t);

    map->stats.total_textures = num_textures;
    map->texture_assets.count = num_textures;
    map->texture_assets.data = gs_malloc(sizeof(gs_asset_texture_t) * num_textures);

    char extensions[2][5] = {
        ".jpg",
        ".tga",
    };

    for (size_t i = 0; i < num_textures; i++)
    {
        bool32_t success = false;
        size_t malloc_sz = strlen(map->textures.data[i].name) + 5;
        char *filename = gs_malloc(malloc_sz);
        memset(filename, 0, malloc_sz);
        strcat(filename, map->textures.data[i].name);
        strcat(filename, extensions[0]);

        for (size_t j = 0; j < 2; j++)
        {
            if (j > 0)
            {
                strcpy(filename + strlen(filename) - 4, extensions[j]);
            }

            if (gs_util_file_exists(filename))
            {
                success = gs_asset_texture_load_from_file(
                    filename,
                    &map->texture_assets.data[i],
                    &(gs_graphics_texture_desc_t){
                        .format = GS_GRAPHICS_TEXTURE_FORMAT_RGBA8,
                        .min_filter = GS_GRAPHICS_TEXTURE_FILTER_NEAREST,
                        .mag_filter = GS_GRAPHICS_TEXTURE_FILTER_NEAREST,
                    },
                    false,
                    false);
            }
            else if (j == 1)
            {
                gs_println("Warning: could not load texture: %s, file not found", map->textures.data[i].name);
            }

            if (success)
            {
                map->stats.loaded_textures++;
                break;
            }
            else
            {
                map->texture_assets.data[i].hndl = gs_handle_invalid(gs_graphics_texture_t);
            }
        }

        gs_free(filename);
    }

#define ROW_COL_CT 10
    // Generate black and pink grid for missing texture
    gs_color_t c0 = gs_color(255, 0, 220, 255);
    gs_color_t c1 = gs_color(0, 0, 0, 255);
    gs_color_t pixels[ROW_COL_CT * ROW_COL_CT] = gs_default_val();
    for (uint32_t r = 0; r < ROW_COL_CT; ++r)
    {
        for (uint32_t c = 0; c < ROW_COL_CT; ++c)
        {
            const bool re = (r % 2) == 0;
            const bool ce = (c % 2) == 0;
            uint32_t idx = r * ROW_COL_CT + c;
            // clang-format off
            pixels[idx] = (re && ce) ? c0 : (re) ? c1 : (ce) ? c1 : c0;
            // clang-format on
        }
    }

    // Create missing texture
    map->missing_texture = gs_graphics_texture_create(
        &(gs_graphics_texture_desc_t){
            .width = ROW_COL_CT,
            .height = ROW_COL_CT,
            .format = GS_GRAPHICS_TEXTURE_FORMAT_RGBA8,
            .min_filter = GS_GRAPHICS_TEXTURE_FILTER_NEAREST,
            .mag_filter = GS_GRAPHICS_TEXTURE_FILTER_NEAREST,
            .data = pixels});
}

void _bsp_load_lightmaps(bsp_map_t *map)
{
}

void _bsp_load_lightvols(bsp_map_t *map)
{
}

void _bsp_create_patch(bsp_map_t *map, bsp_face_lump_t face)
{
    bsp_patch_t patch = {
        .width = face.size[0],
        .height = face.size[1],
        .lightmap_idx = face.lm_index,
        .texture_idx = face.texture,
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
                .tesselation = 10,
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
                    uint32_t vertex_idx = face.first_vertex + (2 * y * patch.width + 2 * x) + (row * patch.width + col);
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

void _bsp_map_create_index_buffer(bsp_map_t *map)
{
    bsp_graphics_index_arr = gs_dyn_array_new(uint32_t);

    for (size_t i = 0; i < gs_dyn_array_size(map->render_faces); i++)
    {
        int32_t index = map->render_faces[i].index;

        if (map->render_faces[i].type == BSP_FACE_TYPE_PATCH)
        {
            bsp_patch_t patch = map->patches[index];
            for (size_t j = 0; j < gs_dyn_array_size(patch.quadratic_patches); j++)
            {
                bsp_quadratic_patch_t quadratic = patch.quadratic_patches[j];
                for (size_t k = 0; k < bsp_quadratic_patch_index_count(&quadratic); k++)
                {
                    gs_dyn_array_push(bsp_graphics_index_arr, quadratic.indices[k]);
                }
            }
        }
        else
        {
            bsp_face_lump_t face = map->faces.data[index];
            int32_t first_index = face.first_index;
            int32_t first_vertex = face.first_vertex;

            for (size_t j = 0; j < face.num_indices; j++)
            {
                gs_dyn_array_push(bsp_graphics_index_arr, first_vertex + map->indices.data[first_index + j].offset);
            }
        }
    }

    bsp_graphics_ibo = gs_graphics_index_buffer_create(
        &(gs_graphics_index_buffer_desc_t){
            .data = bsp_graphics_index_arr,
            .size = sizeof(uint32_t) * gs_dyn_array_size(bsp_graphics_index_arr),
        });
}

void bsp_map_update(bsp_map_t *map, gs_vec3 view_position)
{
    int32_t leaf = _bsp_find_camera_leaf(map, view_position);
    if (leaf == map->previous_leaf)
    {
        // Don't calculate visible faces if no change in leaf
        // TODO: will need to change with frustum culling;
        // make a potentially visible set?
        return;
    }

    _bsp_calculate_visible_faces(map, leaf);
    map->previous_leaf = leaf;
}

void bsp_map_render_immediate(bsp_map_t *map, gs_immediate_draw_t *gsi, gs_camera_t *cam)
{
    gsi_camera(gsi, cam);
    gsi_depth_enabled(gsi, true);
    //gsi_face_cull_enabled(gsi, true);

    for (size_t i = 0; i < gs_dyn_array_size(map->visible_faces); i++)
    {
        int32_t index = map->visible_faces[i].index;

        if (map->visible_faces[i].type == BSP_FACE_TYPE_PATCH)
        {
            bsp_patch_t patch = map->patches[index];

            if (patch.texture_idx >= 0 && gs_handle_is_valid(map->texture_assets.data[patch.texture_idx].hndl))
            {
                gsi_texture(gsi, map->texture_assets.data[patch.texture_idx].hndl);
            }
            else
            {
                gsi_texture(gsi, map->missing_texture);
            }

            for (size_t j = 0; j < gs_dyn_array_size(patch.quadratic_patches); j++)
            {
                bsp_quadratic_patch_t quadratic = patch.quadratic_patches[j];

                for (size_t k = 0; k < bsp_quadratic_patch_index_count(&quadratic) - 2; k += 3)
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
            int32_t first_index = face.first_index;
            int32_t first_vertex = face.first_vertex;

            if (face.texture >= 0 && gs_handle_is_valid(map->texture_assets.data[face.texture].hndl))
            {
                gsi_texture(gsi, map->texture_assets.data[face.texture].hndl);
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

void bsp_map_render(bsp_map_t *map, gs_camera_t *cam)
{
    // Framebuffer size
    const gs_vec2 fbs = gs_platform_framebuffer_sizev(gs_platform_main_window());

    // Clear desc
    gs_graphics_clear_desc_t clear = {
        .actions = &(gs_graphics_clear_action_t){
            .color = {0.1f, 0.1f, 0.1f, 1.0f},
        },
    };

    // Uniforms
    gs_mat4 u_proj = gs_camera_get_view_projection(cam, (s32)fbs.x, (s32)fbs.y);

    // Uniform bind desc
    gs_graphics_bind_uniform_desc_t uniforms[] = {
        (gs_graphics_bind_uniform_desc_t){
            .uniform = bsp_graphics_u_proj,
            .data = &u_proj,
            .binding = 0,
        },
    };

    // Vertex bind desc
    gs_graphics_bind_vertex_buffer_desc_t vbos[] = {
        {.buffer = bsp_graphics_vbo},
    };

    // Index bind desc
    gs_graphics_bind_index_buffer_desc_t ibos[] = {
        {.buffer = bsp_graphics_ibo},
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

    uint32_t sz = gs_dyn_array_size(bsp_graphics_index_arr);

    for (uint32_t i = 0; i < sz; i++)
    {
        uint32_t idx = bsp_graphics_index_arr[i];
        bsp_vert_lump_t vert = map->vertices.data[idx];
        b8 t;
    }

    // Render
    gs_graphics_begin_render_pass(&bsp_graphics_cb, GS_GRAPHICS_RENDER_PASS_DEFAULT);
    gs_graphics_set_viewport(&bsp_graphics_cb, 0, 0, (int32_t)fbs.x, (int32_t)fbs.y);
    gs_graphics_clear(&bsp_graphics_cb, &clear);
    gs_graphics_bind_pipeline(&bsp_graphics_cb, bsp_graphics_pipe);
    gs_graphics_apply_bindings(&bsp_graphics_cb, &binds);
    gs_graphics_draw(&bsp_graphics_cb, &(gs_graphics_draw_desc_t){.start = 0, .count = gs_dyn_array_size(bsp_graphics_index_arr)});
    gs_graphics_end_render_pass(&bsp_graphics_cb);

    // Submit command buffer
    gs_graphics_submit_command_buffer(&bsp_graphics_cb);
}

void bsp_map_free(bsp_map_t *map)
{
    if (map == NULL)
    {
        return;
    }

    gs_dyn_array_free(bsp_graphics_index_arr);

    for (size_t i = 0; i < gs_dyn_array_size(map->patches); i++)
    {
        bsp_patch_free(&map->patches[i]);
    }
    gs_dyn_array_free(map->patches);
    gs_dyn_array_free(map->visible_faces);
    gs_dyn_array_free(map->render_faces);

    map->patches = NULL;
    map->visible_faces = NULL;
    map->render_faces = NULL;

    gs_free(map->texture_assets.data);
    map->texture_assets.data = NULL;

    gs_free(map->entities.ents);
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

    map->entities.ents = NULL;
    map->textures.data = NULL;
    map->planes.data = NULL;
    map->nodes.data = NULL;
    map->leaves.data = NULL;
    map->leaf_faces.data = NULL;
    map->leaf_brushes.data = NULL;
    map->models.data = NULL;
    map->brushes.data = NULL;
    map->brush_sides.data = NULL;
    map->vertices.data = NULL;
    map->indices.data = NULL;
    map->effects.data = NULL;
    map->faces.data = NULL;
    map->lightmaps.data = NULL;
    map->lightvols.data = NULL;
    map->visdata.vecs = NULL;

    gs_free(map);
    map = NULL;
}

int32_t _bsp_find_camera_leaf(bsp_map_t *map, gs_vec3 view_position)
{
    int32_t leaf_index = 0;

    while (leaf_index >= 0)
    {
        bsp_plane_lump_t plane = map->planes.data[map->nodes.data[leaf_index].first_plane];

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

void _bsp_calculate_visible_faces(bsp_map_t *map, int32_t leaf)
{
    gs_dyn_array_clear(map->visible_faces);
    uint32_t visible_patches = 0;
    uint32_t visible_faces = 0;
    int32_t view_cluster = map->leaves.data[leaf].cluster;
    bool32_t cont;

    for (size_t i = 0; i < map->leaves.count; i++)
    {
        bsp_leaf_lump_t lump = map->leaves.data[i];

        if (!_bsp_cluster_visible(map, view_cluster, lump.cluster))
        {
            continue;
        }

        // TODO
        // Frustum culling using lump.mins and lump.maxs

        // Add faces in this leaf to visible set
        for (size_t j = 0; j < lump.num_leaf_faces; j++)
        {
            int32_t idx = map->leaf_faces.data[lump.first_leaf_face + j].face;
            bsp_face_renderable_t face = map->render_faces[idx];

            // Don't add same face multiple times
            // TODO: this probably has terrible perf,
            // use a good set implementation...
            cont = 0;
            for (size_t k = 0; k < gs_dyn_array_size(map->visible_faces); k++)
            {
                if (map->visible_faces[k].index == face.index && map->visible_faces[k].type == face.type)
                {
                    cont = true;
                    break;
                }
            }
            if (cont) continue;

            // TODO billboards
            if (face.type == BSP_FACE_TYPE_BILLBOARD)
            {
                continue;
            }

            gs_dyn_array_push(map->visible_faces, face);

            if (face.type == BSP_FACE_TYPE_PATCH)
            {
                visible_patches++;
            }
            else
            {
                visible_faces++;
            }
        }
    }

    map->stats.visible_faces = visible_faces;
    map->stats.visible_patches = visible_patches;
    map->stats.current_leaf = leaf;
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

    // black magic
    int32_t idx = view_cluster * map->visdata.size_vecs + (test_cluster >> 3);
    return (map->visdata.vecs[idx] & (1 << (test_cluster & 7))) != 0;
}