/*================================================================
    * util/string.h
    *
    * Copyright (c) 2021 Lauri Räsänen
    * ================================

    ...
=================================================================*/

#ifndef MG_STRING_H
#define MG_STRING_H

char *get_filename_from_path(char *path)
{
    char *filename = path;
    for (char *token = path; *token != '\0'; token++)
    {
        if (*token == '/' || *token == '\\')
            filename = token + 1;
    }

    return filename;
}

#endif // MG_STRING_H