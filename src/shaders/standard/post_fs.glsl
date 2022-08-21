/*================================================================
	* shaders/standard/post_fs.glsl
	*
	* Copyright (c) 2021 Lauri Räsänen
	* ================================

	Post-processing effects.

	Barrel distortion based on:
	https://www.decarpentier.nl/lens-distortion
=================================================================*/

#version 330 core

flat in int barrel_enabled;
in vec3 uv;
in vec2 uv_dot;

uniform sampler2D u_tex;
uniform sampler2D u_tex_vm;

out vec4 frag_color;

void main()
{
	vec4 color1;
	vec4 color2;

	if (barrel_enabled == 1)
	{
		vec3 tex_coord = dot(uv_dot, uv_dot) * vec3(-0.5, -0.5, -1.0) + uv;
		color1 = texture2DProj(u_tex, tex_coord);
		color2 = texture2DProj(u_tex_vm, tex_coord);
	}
	else
	{
		color1 = texture(u_tex, uv.xy);
		color2 = texture(u_tex_vm, uv.xy);
	}

	frag_color = mix(color1, color2, color2.a);
}