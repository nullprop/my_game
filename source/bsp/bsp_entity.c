/*================================================================
    * bsp/bsp_entity.c
    *
    * Copyright (c) 2021 Lauri Räsänen
    * ================================

    ...
=================================================================*/

#include <gs/gs.h>

#include "bsp_types.h"

bsp_entity_t bsp_entity_from_string(char *content)
{
    uint32_t sz = gs_string_length(content) + 1;

    bsp_entity_t ent = {
        .slot_map = gs_slot_map_new(char *, char *),
        .content = gs_malloc(sz),
    };

    gs_util_string_remove_character(content, ent.content, sz, '\n');

    _bsp_entity_load_keys(&ent);

    return ent;
}

void bsp_entity_free(bsp_entity_t *ent)
{
    for (
        gs_slot_map_iter it = gs_slot_map_iter_new(ent->slot_map);
        gs_slot_map_iter_valid(ent->slot_map, it);
        gs_slot_map_iter_advance(ent->slot_map, it))
    {
        char *k = gs_slot_map_iter_getk(ent->slot_map, it);
        char *v = gs_slot_map_iter_get(ent->slot_map, it);

        gs_free(k);
        gs_free(v);
        k = NULL;
        v = NULL;
    }

    // FIXME
    //gs_slot_map_free(ent->slot_map);
    gs_free(ent->content);
}

bool bsp_entity_has_key(bsp_entity_t *ent, char *key)
{
    if (ent->slot_map == NULL) return false;

    for (
        gs_slot_map_iter it = gs_slot_map_iter_new(ent->slot_map);
        gs_slot_map_iter_valid(ent->slot_map, it);
        gs_slot_map_iter_advance(ent->slot_map, it))
    {
        char *k = gs_slot_map_iter_getk(ent->slot_map, it);
        if (strcmp(key, k) == 0)
        {
            return true;
        }
    }

    return false;
}

char *bsp_entity_get_value(bsp_entity_t *ent, char *key)
{
    if (ent->slot_map == NULL) return NULL;

    for (
        gs_slot_map_iter it = gs_slot_map_iter_new(ent->slot_map);
        gs_slot_map_iter_valid(ent->slot_map, it);
        gs_slot_map_iter_advance(ent->slot_map, it))
    {
        char *k = gs_slot_map_iter_getk(ent->slot_map, it);
        if (strcmp(key, k) == 0)
        {
            return gs_slot_map_iter_get(ent->slot_map, it);
        }
    }

    return NULL;
}

void bsp_entity_print(bsp_entity_t *ent)
{
    gs_println("====================================");
    gs_println("|            BSP ENTITY             ");
    gs_println("| ----------------------------------");

    for (
        gs_slot_map_iter it = gs_slot_map_iter_new(ent->slot_map);
        gs_slot_map_iter_valid(ent->slot_map, it);
        gs_slot_map_iter_advance(ent->slot_map, it))
    {
        char *k = gs_slot_map_iter_getk(ent->slot_map, it);
        char *v = gs_slot_map_iter_get(ent->slot_map, it);

        gs_println("| %s: %s", k, v);
    }

    gs_println("====================================");
}

void _bsp_entity_load_keys(bsp_entity_t *ent)
{
    static const uint32_t KEY_MAX_SIZE = 128;
    static const uint32_t VAL_MAX_SIZE = 128;

    char state = 0;
    char key[KEY_MAX_SIZE];
    char value[VAL_MAX_SIZE];
    uint32_t key_index = 0;
    uint32_t value_index = 0;

    char c = 0;
    for (size_t i = 0; i < gs_string_length(ent->content); i++)
    {
        c = ent->content[i];

        if (c == '\"')
        {
            if (state == 0)
            {
                // Begin key
                key_index = 0;
            }
            else if (state == 1)
            {
                // End key
                gs_assert(key_index < KEY_MAX_SIZE);
                key[key_index] = '\0';
            }
            else if (state == 2)
            {
                // Begin value
                value_index = 0;
            }
            else if (state == 3)
            {
                // End value, add to map
                gs_assert(value_index < VAL_MAX_SIZE);
                value[value_index] = '\0';

                char *k = gs_malloc(gs_string_length(key) + 1);
                memcpy(k, key, gs_string_length(key) + 1);
                char *v = gs_malloc(gs_string_length(value) + 1);
                memcpy(v, value, gs_string_length(value) + 1);

                gs_slot_map_insert(ent->slot_map, k, v);
            }

            state++;
            if (state >= 4)
                state = 0;
        }
        else
        {
            if (state == 1)
            {
                // Reading key
                gs_assert(key_index < KEY_MAX_SIZE);
                key[key_index] = c;
                key_index++;
            }
            else if (state == 3)
            {
                // Reading value
                gs_assert(value_index < VAL_MAX_SIZE);
                value[value_index] = c;
                value_index++;
            }
        }
    }
}