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

/*================================================================
    BSP shader
=================================================================*/

static const char *mg_bsp_shader_vert_src =
    GL_VERSION_STR
    "layout(location = 0) in vec3 a_pos;\n"
    "layout(location = 1) in vec2 a_tex_coord;\n"
    "layout(location = 2) in vec2 a_lm_coord;\n"
    "layout(location = 3) in vec3 a_normal;\n"
    "layout(location = 4) in vec4 a_color;\n"
    "uniform mat4 u_proj;\n"
    "out vec2 tex_coord;\n"
    "out vec2 lm_coord;\n"
    "\n"
    "void main()\n"
    "{\n"
    "   gl_Position = u_proj * vec4(a_pos, 1.0);\n"
    "   tex_coord = a_tex_coord;\n"
    "   lm_coord = a_lm_coord;\n"
    "}";

static const char *mg_bsp_shader_frag_src =
    GL_VERSION_STR
    "in vec2 tex_coord;\n"
    "in vec2 lm_coord;\n"
    "uniform sampler2D u_tex;\n"
    "uniform sampler2D u_lm;\n"
    "out vec4 frag_color;\n"
    "\n"
    "void main()\n"
    "{\n"
    "   vec4 tex = texture(u_tex, tex_coord);\n"
    "   vec4 lm = texture(u_lm, lm_coord);\n"
    "   \n"
    "   float lm_strength = 2.4f;\n"
    "   float lm_gamma = 1.2f;\n"
    "   \n"
    "   frag_color = tex * lm * lm_strength;\n"
    "   frag_color.rgb = pow(frag_color.rgb, vec3(1.0/lm_gamma));\n"
    "   frag_color.a = 1.0f;\n"
    "}";

#endif // MG_SHADERS_H