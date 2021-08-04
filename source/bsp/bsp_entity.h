/*================================================================
    * bsp/bsp_entity.h
    *
    * Copyright (c) 2021 Lauri Räsänen
    * ================================

    ...
=================================================================*/

#ifndef BSP_ENTITY_H
#define BSP_ENTITY_H

#include <gs/gs.h>

#include "bsp_types.h"

bsp_entity_t bsp_entity_from_string(char *content);
void bsp_entity_free(bsp_entity_t *ent);
bool bsp_entity_has_key(bsp_entity_t *ent, char *key);
char *bsp_entity_get_value(bsp_entity_t *ent, char *key);
void bsp_entity_print(bsp_entity_t *ent);
void _bsp_entity_load_keys(bsp_entity_t *ent);

#endif // BSP_ENTITY_H