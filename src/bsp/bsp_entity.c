/*================================================================
	* bsp/bsp_entity.c
	*
	* Copyright (c) 2021 Lauri Räsänen
	* ================================

	...
=================================================================*/

#include "bsp_entity.h"
#include "../util/console.h"

bsp_entity_t bsp_entity_from_string(char *content)
{
	uint32_t sz = gs_string_length(content) + 1;

	bsp_entity_t ent = {
		.slot_map = gs_slot_map_new(char *, char *),
		.content  = gs_malloc(sz),
	};

	memset(ent.content, 0, sz);
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

	gs_slot_map_free(ent->slot_map);
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
	mg_println("====================================");
	mg_println("|            BSP ENTITY             ");
	mg_println("| ----------------------------------");

	for (
		gs_slot_map_iter it = gs_slot_map_iter_new(ent->slot_map);
		gs_slot_map_iter_valid(ent->slot_map, it);
		gs_slot_map_iter_advance(ent->slot_map, it))
	{
		char *k = gs_slot_map_iter_getk(ent->slot_map, it);
		char *v = gs_slot_map_iter_get(ent->slot_map, it);

		mg_println("| %s: %s", k, v);
	}

	mg_println("====================================");
}

void _bsp_entity_load_keys(bsp_entity_t *ent)
{
#define KEY_MAX_SIZE 128
#define VAL_MAX_SIZE 128

	char state		 = 0;
	char key[KEY_MAX_SIZE]	 = {0};
	char value[VAL_MAX_SIZE] = {0};
	uint32_t key_index	 = 0;
	uint32_t value_index	 = 0;

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

				uint32_t key_len   = gs_string_length(key) + 1;
				uint32_t value_len = gs_string_length(value) + 1;

				char *k = gs_malloc(key_len);
				memcpy(k, key, key_len);
				char *v = gs_malloc(value_len);
				memcpy(v, value, value_len);

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