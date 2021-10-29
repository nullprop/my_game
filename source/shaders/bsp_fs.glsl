/*================================================================
    * shaders/bsp_fs.glsl
    *
    * Copyright (c) 2021 Lauri Räsänen
    * ================================

    BSP fragment shader.
    Texture + lightmaps.
=================================================================*/

#version 330 core

in vec2 tex_coord;
in vec2 lm_coord;

uniform sampler2D u_tex;
uniform sampler2D u_lm;

out vec4 frag_color;

void main()
{
    vec4 tex = texture(u_tex, tex_coord);
    vec4 lm = texture(u_lm, lm_coord);
   
    // magic values for the look I want
    float lm_strength = 2.8;
    float gamma = 1.15;
   
    frag_color = tex * lm * lm_strength;
    frag_color.rgb = pow(frag_color.rgb, vec3(1.0/gamma));
    frag_color.a = 1.0;
}