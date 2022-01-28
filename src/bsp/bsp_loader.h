/*================================================================
	* bsp/bsp_loader.h
	*
	* Copyright (c) 2021 Lauri Räsänen
	* ================================

	...
=================================================================*/

#ifndef BSP_LOADER_H
#define BSP_LOADER_H

#include <gs/gs.h>

#include "../util/string.h"
#include "bsp_types.h"

b32 _load_bsp_fail(gs_byte_buffer_t *buffer, char *msg);
b32 load_bsp(char *filename, bsp_map_t *map);
b32 _load_bsp_header(gs_byte_buffer_t *buffer, bsp_header_t *header);
b32 _load_lump(gs_byte_buffer_t *buffer, bsp_map_t *map, uint32_t *count, void **data, bsp_lump_types type, uint32_t lump_size);
b32 _load_entity_lump(gs_byte_buffer_t *buffer, bsp_map_t *map);
b32 _load_visdata_lump(gs_byte_buffer_t *buffer, bsp_map_t *map);

#endif // BSP_LOADER_H