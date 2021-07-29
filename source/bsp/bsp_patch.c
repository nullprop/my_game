/*================================================================
    * bsp/bsp_patch.c
    *
    * Copyright (c) 2021 Lauri Räsänen
    * Copyright (c) 2018 Krzysztof Kondrak
    * 
    * See README.md for license.
    * ================================

    Handles BSP patch tesselation.
=================================================================*/

#include "bsp_types.h"

// Helpers for vertex lump math
bsp_vert_lump_t bsp_vert_lump_mul(bsp_vert_lump_t lump, float32_t mul)
{
    bsp_vert_lump_t result = {
        .position = gs_vec3_scale(lump.position, mul),
        .tex_coord = gs_vec2_scale(lump.tex_coord, mul),
        .lm_coord = gs_vec2_scale(lump.lm_coord, mul),
        .normal = lump.normal,
        .color = lump.color};
    return result;
}

bsp_vert_lump_t bsp_vert_lump_add(bsp_vert_lump_t a, bsp_vert_lump_t b)
{
    bsp_vert_lump_t result = {
        .position = gs_vec3_add(a.position, b.position),
        .tex_coord = gs_vec2_add(a.tex_coord, b.tex_coord),
        .lm_coord = gs_vec2_add(a.lm_coord, b.lm_coord),
        .normal = a.normal,
        .color = a.color};
    return result;
}

// Get count of indices in quadratic patch
uint32_t bsp_quadratic_patch_index_count(bsp_quadratic_patch_t *patch)
{
    return patch->tesselation * patch->tesselation * 6;
}

// Tesselate quadratic patch to wanted level
void bsp_quadratic_patch_tesselate(bsp_quadratic_patch_t *patch)
{
    gs_dyn_array_reserve(patch->vertices, (patch->tesselation + 1) * (patch->tesselation + 1));

    // Sample (tesselation + 1)^2 points
    // for our vertices from the bezier patch.
    for (size_t i = 0; i <= patch->tesselation; i++)
    {
        // How far into the row are we?
        float32_t a = (float32_t)i / patch->tesselation;
        float32_t b = 1.0f - a;

        // Sample each row into temp variable
        bsp_vert_lump_t temp[3];
        for (size_t j = 0; j < 3; j++)
        {
            int32_t k = j * 3;

            // cp[k + 0] * (b * b) +
            // cp[k + 1] * (2 * b * b) +
            // cp[k + 2] * (a * a)
            temp[j] = bsp_vert_lump_add(
                bsp_vert_lump_mul(patch->control_points[k + 0], (b * b)),
                bsp_vert_lump_add(
                    bsp_vert_lump_mul(patch->control_points[k + 1], (2 * b * a)),
                    bsp_vert_lump_mul(patch->control_points[k + 2], (a * a))));
        }

        // Sample each column using row values
        for (size_t j = 0; j <= patch->tesselation; j++)
        {
            // How far into the column are we?
            a = (float32_t)j / patch->tesselation;
            b = 1.0f - a;

            // temp[k + 0] * (b * b) +
            // temp[k + 1] * (2 * b * b) +
            // temp[k + 2] * (a * a)
            bsp_vert_lump_t val = bsp_vert_lump_add(
                bsp_vert_lump_mul(temp[0], b * b),
                bsp_vert_lump_add(
                    bsp_vert_lump_mul(temp[1], 2 * b * a),
                    bsp_vert_lump_mul(temp[2], a * a)));

            gs_dyn_array_set_data_i(&patch->vertices, &val, sizeof(bsp_vert_lump_t), i * (patch->tesselation + 1) + j);
        }
    }

    // Triangulate
    gs_dyn_array_reserve(patch->indices, bsp_quadratic_patch_index_count(patch));
    for (size_t row = 0; row < patch->tesselation; row++)
    {
        for (size_t col = 0; col < patch->tesselation; col++)
        {
            gs_dyn_array_push(patch->indices, (row + 1) * (patch->tesselation + 1) + col);
            gs_dyn_array_push(patch->indices, (row + 0) * (patch->tesselation + 1) + col);
            gs_dyn_array_push(patch->indices, (row + 1) * (patch->tesselation + 1) + col + 1);

            gs_dyn_array_push(patch->indices, (row + 1) * (patch->tesselation + 1) + col + 1);
            gs_dyn_array_push(patch->indices, (row + 0) * (patch->tesselation + 1) + col);
            gs_dyn_array_push(patch->indices, (row + 0) * (patch->tesselation + 1) + col + 1);
        }
    }
}

void bsp_quadratic_patch_free(bsp_quadratic_patch_t *patch)
{
    gs_dyn_array_free(patch->vertices);
    patch->vertices = NULL;
    gs_dyn_array_free(patch->indices);
    patch->indices = NULL;
}

void bsp_patch_free(bsp_patch_t *patch)
{
    for (size_t i = 0; i < gs_dyn_array_size(patch->quadratic_patches); i++)
    {
        bsp_quadratic_patch_free(&patch->quadratic_patches[i]);
    }

    gs_dyn_array_free(patch->quadratic_patches);
    patch->quadratic_patches = NULL;
}