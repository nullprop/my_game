/*================================================================
    * util/string.h
    *
    * Copyright (c) 2021 Lauri Räsänen
    * ================================

    ...
=================================================================*/

#ifndef MG_STRING_H
#define MG_STRING_H

#include <gs/gs.h>

static inline char *mg_duplicate_string(char *str)
{
    uint32_t sz = gs_string_length(str) + 1;
    char *dup = gs_malloc(sz);
    memcpy(dup, str, sz);
    return dup;
}

static inline char *mg_append_string(char *str1, char *str2)
{
    uint32_t sz1 = gs_string_length(str1);
    uint32_t sz2 = gs_string_length(str2);
    char *app = gs_malloc(sz1 + sz2 + 1);
    memcpy(app, str1, sz1);
    memcpy(app + sz1, str2, sz2);
    app[sz1 + sz2] = '\0';
    return app;
}

static inline char *mg_get_filename_from_path(char *path)
{
    char *filename = path;

    // Move pointer to last separator
    for (char *token = path; *token != '\0'; token++)
    {
        if (*token == '/' || *token == '\\')
            filename = token + 1;
    }

    return filename;
}

static inline char *mg_get_directory_from_path(char *path)
{
    char *dir = mg_duplicate_string(path);

    // null-terminate path after last separator
    for (char *token = dir + gs_string_length(dir); token >= dir; token--)
    {
        if (*token == '/' || *token == '\\')
        {
            break;
        }

        *token = '\0';
    }

    return dir;
}

#endif // MG_STRING_H