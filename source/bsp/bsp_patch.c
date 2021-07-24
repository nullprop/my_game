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
        .position = gs_vec3_mul(lump.position, gs_v3(mul, mul, mul)),
        .tex_coord = gs_vec2_mul(lump.tex_coord, gs_v2(mul, mul)),
        .lm_coord = gs_vec2_mul(lump.lm_coord, gs_v2(mul, mul)),
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

// Tesselate quadratic patch to wanted level
void bsp_quadratic_patch_tesselate(bsp_quadratic_patch_t *patch)
{
    gs_dyn_array_resize_impl(&patch->vertices, sizeof(bsp_vert_lump_t), (patch->tesselation + 1) * (patch->tesselation + 1));

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
    gs_dyn_array_resize_impl(&patch->indices, sizeof(uint16_t), patch->tesselation * patch->tesselation * 6);
    for (size_t row = 0; row < patch->tesselation; row++)
    {
        for (size_t col = 0; col < patch->tesselation; col++)
        {
            uint16_t temp1 = (row + 1) * (patch->tesselation + 1) + col;
            uint16_t temp2 = (row + 0) * (patch->tesselation + 1) + col;
            uint16_t temp3 = (row + 1) * (patch->tesselation + 1) + col + 1;
            gs_dyn_array_push_data(&patch->indices, &temp1, sizeof(uint16_t));
            gs_dyn_array_push_data(&patch->indices, &temp2, sizeof(uint16_t));
            gs_dyn_array_push_data(&patch->indices, &temp3, sizeof(uint16_t));

            temp1 = (row + 1) * (patch->tesselation + 1) + col + 1;
            temp2 = (row + 0) * (patch->tesselation + 1) + col;
            temp3 = (row + 0) * (patch->tesselation + 1) + col + 1;
            gs_dyn_array_push_data(&patch->indices, &temp1, sizeof(uint16_t));
            gs_dyn_array_push_data(&patch->indices, &temp2, sizeof(uint16_t));
            gs_dyn_array_push_data(&patch->indices, &temp3, sizeof(uint16_t));
        }
    }
}