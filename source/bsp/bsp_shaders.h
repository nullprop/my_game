#ifndef BSP_SHADERS_H
#define BSP_SHADERS_H

#define GS_VERSION_STR "#version 330 core\n"

const char *bsp_shader_vert_src =
    GS_VERSION_STR
    "layout(location = 0) in vec3 a_pos;\n"
    "layout(location = 1) in vec2 a_tex_coord;\n"
    "layout(location = 2) in vec2 a_lm_coord;\n"
    "layout(location = 3) in vec3 a_normal;\n"
    "layout(location = 4) in vec4 a_color;\n"
    "out vec4 color;\n"
    "uniform mat4 u_proj;\n"
    "void main()\n"
    "{\n"
    "   gl_Position = u_proj * vec4(a_pos, 1.0);\n"
    "   color = a_color;\n"
    "}";

const char *bsp_shader_frag_src =
    GS_VERSION_STR
    "in vec4 color;\n"
    "out vec4 frag_color;\n"
    "void main()\n"
    "{\n"
    "   frag_color = color;\n"
    "}";

#endif // BSP_SHADERS_H