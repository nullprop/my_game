#ifndef MG_CONSOLE_H
#define MG_CONSOLE_H

#include <gs/gs.h>

#define MG_CON_LINES 512
#define MG_CON_HIST  64

typedef enum mg_cmd_arg_type
{
	MG_CMD_ARG_STRING,
	MG_CMD_ARG_INT,
	MG_CMD_ARG_FLOAT,
	MG_CMD_ARG_COUNT,
} mg_cmd_arg_type;

typedef struct mg_cmd_t
{
	char *name;
	char *help;
	void (*func)(void **);
	mg_cmd_arg_type *argt;
	uint8_t argc;
} mg_cmd_t;

typedef struct mg_console_t
{
	char *output[MG_CON_LINES];
	char *input[MG_CON_HIST];
	gs_dyn_array(mg_cmd_t) commands;
} mg_console_t;

void mg_console_init();
void mg_console_free();

void mg_console_println(const char *text);
void mg_console_input(const char *text);
void mg_console_run(const mg_cmd_t cmd, void **argv);
char *mg_console_get_last(char **container, size_t container_len, size_t sz);

void mg_console_clear();
void mg_console_help();

extern mg_console_t *g_console;

#define mg_println(__FMT, ...)                                  \
	{                                                       \
		gs_printf(__FMT, ##__VA_ARGS__);                \
		gs_printf("\n");                                \
                                                                \
		char *tmp = gs_malloc(1024);                    \
		gs_snprintf(tmp, 1024, __FMT, ##__VA_ARGS__);   \
		if (g_console != NULL) mg_console_println(tmp); \
		gs_free(tmp);                                   \
	}

#define mg_cmd_new(n, h, f, t, c)                                                                    \
	{                                                                                            \
		mg_cmd_t cmd = (mg_cmd_t){.name = n, .help = h, .func = f, .argt = NULL, .argc = c}; \
		if (c > 0)                                                                           \
		{                                                                                    \
			cmd.argt = gs_malloc(c * sizeof(mg_cmd_arg_type));                           \
			memcpy(cmd.argt, t, c * sizeof(mg_cmd_arg_type));                            \
		}                                                                                    \
		gs_dyn_array_push(g_console->commands, cmd);                                         \
	}

#endif // MG_CONSOLE_H