#ifndef MG_CONSOLE_H
#define MG_CONSOLE_H

#include <gs/gs.h>

#define MG_CON_LINES 512
#define MG_CON_HIST  64

typedef struct mg_console_t
{
	char *output[MG_CON_LINES];
	char *input[MG_CON_HIST];
} mg_console_t;

void mg_console_init();
void mg_console_free();

void mg_console_println(const char *text);
void mg_console_run(const char *text);
char *mg_console_get_last(char **container, size_t container_len, size_t sz);

void mg_console_clear();

extern mg_console_t *g_console;

#define mg_println(__FMT, ...)                                \
	{                                                     \
		gs_printf(__FMT, ##__VA_ARGS__);              \
		gs_printf("\n");                              \
                                                              \
		char *tmp = gs_malloc(1024);                  \
		gs_snprintf(tmp, 1024, __FMT, ##__VA_ARGS__); \
		mg_console_println(tmp);                      \
		gs_free(tmp);                                 \
	}

#endif // MG_CONSOLE_H