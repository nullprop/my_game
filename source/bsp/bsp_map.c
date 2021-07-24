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

#include "bsp_types.h"

void bsp_map_init(bsp_map_t *map)
{
    if (map->faces.count == 0)
    {
        return;
    }

    gs_dyn_array_init(&map->patches, sizeof(bsp_patch_t));
}

void bsp_map_update(bsp_map_t *map)
{
}

void bsp_map_render(bsp_map_t *map)
{
}