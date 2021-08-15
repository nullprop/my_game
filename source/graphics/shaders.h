/*================================================================
    * graphics/shaders.h
    *
    * Copyright (c) 2021 Lauri Räsänen
    * ================================

    ...
=================================================================*/

#ifndef MG_SHADERS_H
#define MG_SHADERS_H

#define GL_VERSION_STR "#version 330 core\n"

static const char *mg_shader_vert_src =
    GL_VERSION_STR
    "layout(location = 0) in vec3 a_pos;\n"
    "uniform mat4 u_proj;\n"
    "\n"
    "void main()\n"
    "{\n"
    "   gl_Position = u_proj * vec4(a_pos, 1.0);\n"
    "}";

static const char *mg_shader_frag_src =
    GL_VERSION_STR
    "out vec4 frag_color;\n"
    "\n"
    "void main()\n"
    "{\n"
    "   frag_color = vec4(0.5f, 0.5f, 0.5f, 1.0f);\n"
    "}";

#endif // MG_SHADERS_H