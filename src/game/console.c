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
	g_console->commands = gs_dyn_array_new(mg_cmd_t);

	mg_cmd_new("clear", "Clears the console", &mg_console_clear, NULL, 0);
	mg_cmd_new("help", "Shows available commands", &mg_console_help, NULL, 0);
	mg_cmd_new("exit", "Exits the game", &gs_quit, NULL, 0);
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
	for (size_t i = 0; i < gs_dyn_array_size(g_console->commands); i++)
	{
		gs_free(g_console->commands[i].name);
		gs_free(g_console->commands[i].help);
		if (g_console->commands[i].argt != NULL)
		{
			gs_free(g_console->commands[i].argt);
		}
	}
	gs_free(g_console->commands);
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

void mg_console_input(const char *text)
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

	char *token = strtok(text, " ");
	if (!token)
	{
		mg_println("ERR: Failed to tokenize console input %s", text);
		return;
	}

	// Handle commands
	for (size_t i = 0; i < gs_dyn_array_size(g_console->commands); i++)
	{
		mg_cmd_t cmd = g_console->commands[i];
		if (strcmp(text, cmd.name) == 0)
		{
			if (cmd.argc == 0)
			{
				mg_console_run(cmd, NULL);
			}
			else
			{
				void **argv = gs_malloc(cmd.argc * sizeof(void *));
				for (size_t j = 0; j < cmd.argc; j++)
				{
					token = strtok(NULL, " ");
					if (!token)
					{
						mg_println("ERR: Not enough arguments for command '%s'. Expected %d.", cmd.argc);
						for (size_t k = 0; k < j; k++)
						{
							gs_free(argv[k]);
						}

						gs_free(argv);
						return;
					}
					mg_cmd_arg_type ttt = cmd.argt[j];
					switch (cmd.argt[j])
					{
					default:
					case MG_CMD_ARG_STRING:
						sz	= gs_string_length(token) + 1;
						argv[j] = gs_malloc(sz);
						memcpy(argv[j], token, sz);
						break;

					case MG_CMD_ARG_INT:
						argv[j]		= gs_malloc(sizeof(int));
						*(int *)argv[j] = atoi(token);
						break;

					case MG_CMD_ARG_FLOAT:
						argv[j]		  = gs_malloc(sizeof(float));
						*(float *)argv[j] = atof(token);
						break;
					}
				}

				mg_console_run(cmd, argv);

				for (size_t j = 0; j < cmd.argc; j++)
				{
					gs_free(argv[j]);
				}
				gs_free(argv);
			}

			return;
		}
	}

	// Handle cvars
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

	mg_println("Unknown command or cvar '%s'. Type 'help' to show available commands.", text);
}

void mg_console_run(const mg_cmd_t cmd, void **argv)
{
	if (cmd.func == NULL)
	{
		mg_println("ERR: Command '%s' doesn't have a function", cmd.name);
		return;
	}

	// Too lazy for vargs, shouldn't need more than this...
	if (cmd.argc == 0)
	{
		(cmd.func)(NULL);
	}
	else if (cmd.argc == 1)
	{
		void (*tmp)(void *) = cmd.func;
		(tmp)(argv[0]);
	}
	else if (cmd.argc == 2)
	{
		void (*tmp)(void *, void *) = cmd.func;
		(tmp)(argv[0], argv[1]);
	}
	else if (cmd.argc == 3)
	{
		void (*tmp)(void *, void *, void *) = cmd.func;
		(tmp)(argv[0], argv[1], argv[1]);
	}
	else
	{
		mg_println("WARN: not unpacking arguments for command '%s'", cmd.name);
		(cmd.func)(argv);
	}
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

void mg_console_help()
{
	mg_println("help:");
	for (size_t i = 0; i < gs_dyn_array_size(g_console->commands); i++)
	{
		mg_println("  '%s' - %s", g_console->commands[i].name, g_console->commands[i].help);
	}
}