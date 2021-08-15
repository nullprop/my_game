/*================================================================
    * graphics/renderer.c
    *
    * Copyright (c) 2021 Lauri Räsänen
    * ================================

    ...
=================================================================*/

#include "renderer.h"
#include "../util/camera.h"
#include "shaders.h"

mg_renderer_t *g_renderer;

void mg_renderer_init()
{
    // Allocate
    g_renderer = gs_malloc_init(mg_renderer_t);
    g_renderer->cb = gs_command_buffer_new();
    g_renderer->gsi = gs_immediate_draw_new();
    g_renderer->renderables = gs_slot_array_new(mg_renderable_t);

    // Shader source description
    gs_graphics_shader_source_desc_t sources[] = {
        (gs_graphics_shader_source_desc_t){.type = GS_GRAPHICS_SHADER_STAGE_VERTEX, .source = mg_shader_vert_src},
        (gs_graphics_shader_source_desc_t){.type = GS_GRAPHICS_SHADER_STAGE_FRAGMENT, .source = mg_shader_frag_src},
    };

    // Create shader
    g_renderer->shader = gs_graphics_shader_create(
        &(gs_graphics_shader_desc_t){
            .sources = sources,
            .size = sizeof(sources),
            .name = "model",
        });

    // Create uniforms
    g_renderer->u_proj = gs_graphics_uniform_create(
        &(gs_graphics_uniform_desc_t){
            .name = "u_proj",
            .layout = &(gs_graphics_uniform_layout_desc_t){
                .type = GS_GRAPHICS_UNIFORM_MAT4,
            },
            .stage = GS_GRAPHICS_SHADER_STAGE_VERTEX,
        });

    // Pipeline vertex attributes
    gs_graphics_vertex_attribute_desc_t vattrs[] = {
        (gs_graphics_vertex_attribute_desc_t){.format = GS_GRAPHICS_VERTEX_ATTRIBUTE_FLOAT3, .name = "a_pos"},
    };

    // Create pipeline
    g_renderer->pipe = gs_graphics_pipeline_create(
        &(gs_graphics_pipeline_desc_t){
            .raster = {
                .shader = g_renderer->shader,
                .index_buffer_element_size = sizeof(uint32_t),
            },
            .blend = {
                .func = GS_GRAPHICS_BLEND_EQUATION_ADD,
                .src = GS_GRAPHICS_BLEND_MODE_SRC_ALPHA,
                .dst = GS_GRAPHICS_BLEND_MODE_ONE_MINUS_SRC_ALPHA,
            },
            .depth = {
                .func = GS_GRAPHICS_DEPTH_FUNC_LESS,
            },
            .layout = {
                .attrs = vattrs,
                .size = sizeof(vattrs),
            },
        });
}

void mg_renderer_update()
{
    // Framebuffer size
    const gs_vec2 fb = gs_platform_framebuffer_sizev(gs_platform_main_window());

    // Render passes
    // TODO: bsp should be rendered here eventually as well
    _mg_renderer_renderable_pass(fb);
    _mg_renderer_immediate_pass(fb);

    // Submit command buffer (syncs to GPU, MUST be done on main thread where you have your GPU context created)
    gs_graphics_submit_command_buffer(&g_renderer->cb);
}

void mg_renderer_free()
{
    gs_immediate_draw_free(&g_renderer->gsi);
    gs_command_buffer_free(&g_renderer->cb);
}

uint32_t mg_renderer_create_renderable(mg_model_t model, gs_vqs *transform)
{
    mg_renderable_t renderable = {
        .model = model,
        .transform = transform,
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
            .data = &u_proj,
            .binding = 0, // VERTEX
        },
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

        // Draw all primitives in renderable
        for (uint32_t i = 0; i < gs_dyn_array_size(renderable->model.data.primitives); i++)
        {
            gs_gfxt_mesh_primitive_t *prim = &renderable->model.data.primitives[i];

            // Construct binds
            gs_graphics_bind_desc_t binds = {
                .vertex_buffers = {
                    .desc = &(gs_graphics_bind_vertex_buffer_desc_t){
                        .buffer = prim->vbo,
                    },
                },
                .index_buffers = {
                    .desc = &(gs_graphics_bind_index_buffer_desc_t){
                        .buffer = prim->ibo,
                    },
                },
                .uniforms = {
                    .desc = uniforms,
                    .size = sizeof(uniforms),
                },
            };

            gs_graphics_apply_bindings(&g_renderer->cb, &binds);
            gs_graphics_draw(&g_renderer->cb, &(gs_graphics_draw_desc_t){.start = 0, .count = prim->count});
        }
    }

    gs_graphics_end_render_pass(&g_renderer->cb);
}

void _mg_renderer_immediate_pass(gs_vec2 fb)
{
    if (!g_renderer->use_immediate_mode) return;

    gs_renderpass im_pass = gs_default_val();
    gs_graphics_begin_render_pass(&g_renderer->cb, im_pass);
    gs_graphics_set_viewport(&g_renderer->cb, 0, 0, (int32_t)fb.x, (int32_t)fb.y);
    gsi_draw(&g_renderer->gsi, &g_renderer->cb);
    gs_graphics_end_render_pass(&g_renderer->cb);
}