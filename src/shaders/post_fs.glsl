/*================================================================
	* shaders/post_fs.glsl
	*
	* Copyright (c) 2021 Lauri Räsänen
	* ================================

	Post-processing effects.

	Barrel distortion based on:
	https://www.decarpentier.nl/lens-distortion
=================================================================*/

#version 330 core

in vec3 uv;
in vec2 uv_dot;

uniform sampler2D u_tex;

out vec4 frag_color;

void main()
{
	vec3 tex_coord = dot(uv_dot, uv_dot) * vec3(-0.5, -0.5, -1.0) + uv;
	frag_color = texture2DProj(u_tex, tex_coord);
}