#include "console.h"
#include "config.h"

mg_console_t *g_console;

void mg_console_init()
{
	g_console = gs_malloc(sizeof(mg_console_t));
	for (size_t i = 0; i < MG_CON_LINES; i++)
	{
		g_console->output[i] = NULL;
	}
	for (size_t i = 0; i < MG_CON_HIST; i++)
	{
		g_console->input[i] = NULL;
	}
}

void mg_console_free()
{
	for (size_t i = 0; i < MG_CON_LINES; i++)
	{
		gs_free(g_console->output[i]);
	}
	for (size_t i = 0; i < MG_CON_HIST; i++)
	{
		gs_free(g_console->input[i]);
	}
	gs_free(g_console);
}

void mg_console_println(const char *text)
{
	size_t sz    = gs_string_length(text) + 1;
	char *target = mg_console_get_last(g_console->output, MG_CON_LINES, sz);
	if (target != NULL)
	{
		memcpy(target, text, sz);
	}
}

void mg_console_run(const char *text)
{
	// Store in input history
	size_t sz    = gs_string_length(text) + 1;
	char *target = mg_console_get_last(g_console->input, MG_CON_HIST, sz);
	if (target != NULL)
	{
		memcpy(target, text, sz);
	}

	// Store in output history, add prefix
	size_t tmp_sz = sz + 2;
	char *tmp     = gs_malloc(tmp_sz);
	tmp[0]	      = '>';
	tmp[1]	      = ' ';
	memcpy(&tmp[2], text, sz);
	target = mg_console_get_last(g_console->output, MG_CON_LINES, tmp_sz);
	if (target != NULL)
	{
		memcpy(target, tmp, tmp_sz);
	}
	gs_free(tmp);

	// Handle commands
	// TODO command definitions similar to mg_cvar?
	//      Command name,
	//      function to execute,
	//      help text,
	//      aliases
	if (strcmp(text, "clear") == 0)
	{
		mg_console_clear();
		return;
	}
	else if (strcmp(text, "exit") == 0)
	{
		gs_quit();
		return;
	}
	else if (strcmp(text, "help") == 0)
	{
		mg_println("commands:");
		mg_println("  'cvars' - Shows a list of available cvars");
		mg_println("  'clear' - Clear the console");
		mg_println("  'exit'  - Exits the game");
		return;
	}
	else if (strcmp(text, "cvars") == 0)
	{
		mg_config_print();
		return;
	}

	// Handle cvars
	char *token = strtok(text, " ");
	if (token)
	{
		mg_cvar_t *cvar = mg_cvar(token);
		if (cvar)
		{
			token = strtok(NULL, " ");
			if (token)
			{
				mg_cvar_set(cvar, token);
			}
			mg_cvar_print(cvar);
			return;
		}
	}

	mg_println("Unknown command or cvar '%s'. Type 'help' to show available commands.", text);
}

char *mg_console_get_last(char **container, size_t container_len, size_t sz)
{
	if (container[0] != NULL)
	{
		// Container is full, shift all lines by 1
		char *tmp = container[container_len - 1];
		memcpy(&container[1], &container[0], sizeof(char *) * container_len - 1);
		container[0] = gs_realloc(tmp, sz);
		return container[0];
	}
	else
	{
		// Find first unused line
		for (size_t i = container_len - 1; i >= 0; i--)
		{
			if (container[i] == NULL)
			{
				container[i] = gs_malloc(sz);
				return container[i];
			}
		}
	}

	return NULL;
}

void mg_console_clear()
{
	for (size_t i = 0; i < MG_CON_LINES; i++)
	{
		gs_free(g_console->output[i]);
		g_console->output[i] = NULL;
	}
}