/*================================================================
    * util/string.h
    *
    * Copyright (c) 2021 Lauri Räsänen
    * ================================

    ...
=================================================================*/

#ifndef MG_STRING_H
#define MG_STRING_H

char *mg_get_filename_from_path(char *path)
{
    char *filename = path;
    for (char *token = path; *token != '\0'; token++)
    {
        if (*token == '/' || *token == '\\')
            filename = token + 1;
    }

    return filename;
}

char *mg_duplicate_string(char *str)
{
    uint32_t sz = gs_string_length(str) + 1;
    char *dup = gs_malloc(sz);
    memcpy(dup, str, sz);
    return dup;
}

#endif // MG_STRING_H