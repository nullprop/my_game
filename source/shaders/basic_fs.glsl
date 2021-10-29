/*================================================================
    * shaders/basic_fs.glsl
    *
    * Copyright (c) 2021 Lauri Räsänen
    * ================================

    Basic lit fragment shader with texture.
    1 directional light + ambient.
=================================================================*/

#version 330 core

in vec3 v_normal;
in vec2 v_texcoord;

struct Light
{
    vec3 ambient;
    vec3 directional;
    vec3 direction;
};

uniform Light u_light;
uniform sampler2D u_tex;

out vec4 frag_color;

void main()
{
    vec4 albedo = texture(u_tex, v_texcoord);

    // magic values for the look I want
    float directional_strength = 2.4;
    float ambient_strength = 1.0;
    float gamma = 1.1;

    // Directional
    float light_dot = max(0, dot(v_normal, u_light.direction));
    vec3 lighting = u_light.directional * light_dot * directional_strength;

    // Ambient
    lighting += u_light.ambient * ambient_strength;

    frag_color = albedo * vec4(lighting, 1.0);
    frag_color.rgb = pow(frag_color.rgb, vec3(1.0 / gamma));
}