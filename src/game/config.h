/*================================================================
	* game/config.h
	*
	* Copyright (c) 2021 Lauri Räsänen
	* ================================

	Config types.
=================================================================*/

#ifndef MG_CONFIG_H
#define MG_CONFIG_H

#include "console.h"
#include <gs/gs.h>

#define MG_CVAR_STR_LEN 64

typedef enum mg_cvar_type
{
	MG_CONFIG_TYPE_INT,
	MG_CONFIG_TYPE_FLOAT,
	MG_CONFIG_TYPE_STRING,
	MG_CONFIG_TYPE_COUNT,
} mg_cvar_type;

typedef struct mg_cvar_t
{
	char name[MG_CVAR_STR_LEN];
	mg_cvar_type type;
	union
	{
		float32_t f;
		int32_t i;
		char *s;
	} value;
} mg_cvar_t;

typedef struct mg_config_t
{
	gs_dyn_array(mg_cvar_t) cvars;
} mg_config_t;

void mg_config_init();
void mg_config_free();
mg_cvar_t *mg_config_get(char *name);
void mg_config_print();
void _mg_config_load(char *filepath);
void _mg_config_save(char *filepath);

extern mg_config_t *g_config;

#define mg_cvar_new(n, t, v)                                                    \
	{                                                                       \
		mg_cvar_t cvar = (mg_cvar_t){.name = n, .type = t, .value = 0}; \
		if (t == MG_CONFIG_TYPE_INT)                                    \
		{                                                               \
			cvar.value.i = v;                                       \
		}                                                               \
		else if (t == MG_CONFIG_TYPE_FLOAT)                             \
		{                                                               \
			cvar.value.f = v;                                       \
		}                                                               \
		else                                                            \
		{                                                               \
			gs_assert(false);                                       \
		}                                                               \
		gs_dyn_array_push(g_config->cvars, cvar);                       \
	}

// painful to make into a single macro because of float -> char* casting
#define mg_cvar_new_str(n, t, v)                                                           \
	{                                                                                  \
		mg_cvar_t cvar = (mg_cvar_t){.name = n, .type = t, .value = {0}};          \
		cvar.value.s   = gs_malloc(MG_CVAR_STR_LEN);                               \
		memset(cvar.value.s, 0, MG_CVAR_STR_LEN);                                  \
		memcpy(cvar.value.s, v, gs_min(MG_CVAR_STR_LEN - 1, gs_string_length(v))); \
		gs_dyn_array_push(g_config->cvars, cvar);                                  \
	}

#define mg_cvar(n) \
	mg_config_get(n)

#define mg_cvar_print(cvar)                                                   \
	{                                                                     \
		switch ((cvar)->type)                                         \
		{                                                             \
		default:                                                      \
		case MG_CONFIG_TYPE_STRING:                                   \
			mg_println("%s = %s", (cvar)->name, (cvar)->value.s); \
			break;                                                \
                                                                              \
		case MG_CONFIG_TYPE_FLOAT:                                    \
			mg_println("%s = %f", (cvar)->name, (cvar)->value.f); \
			break;                                                \
                                                                              \
		case MG_CONFIG_TYPE_INT:                                      \
			mg_println("%s = %d", (cvar)->name, (cvar)->value.i); \
			break;                                                \
		};                                                            \
	}

#define mg_cvar_set(cvar, str)                                                   \
	{                                                                        \
		switch ((cvar)->type)                                            \
		{                                                                \
		default:                                                         \
		case MG_CONFIG_TYPE_STRING:                                      \
			memcpy((cvar)->value.s, str, gs_string_length(str) + 1); \
			break;                                                   \
                                                                                 \
		case MG_CONFIG_TYPE_FLOAT:                                       \
			(cvar)->value.f = atof(str);                             \
			break;                                                   \
                                                                                 \
		case MG_CONFIG_TYPE_INT:                                         \
			(cvar)->value.i = atoi(str);                             \
			break;                                                   \
		};                                                               \
	}

#endif // MG_CONFIG_H